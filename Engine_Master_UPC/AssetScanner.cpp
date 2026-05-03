#include "Globals.h"
#include "AssetScanner.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "Importer.h"
#include "Asset.h"
#include "Metadata.h"
#include "UID.h"
#include "MD5.h"
#include "ThreadPool.h"

#include <FileIO.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <future>
#include <mutex>
#include <vector>

constexpr bool ASSETS_MY_DEBUG = false;

#define DEBUG_ASSETS(...) do { if constexpr (ASSETS_MY_DEBUG) { DEBUG_LOG(__VA_ARGS__); } } while (0)

namespace
{
    struct ScanStats
    {
        size_t numFiles = 0;
        size_t numMetadata = 0;
        size_t numMD5 = 0;
        size_t numMissingMetadata = 0;
        size_t numImportsQueued = 0;
    };

    thread_local ScanStats g_threadStats;

    double elapsedMs(
        const std::chrono::high_resolution_clock::time_point& begin,
        const std::chrono::high_resolution_clock::time_point& end)
    {
        return std::chrono::duration<double, std::milli>(end - begin).count();
    }
}

AssetScanner::AssetScanner()
{
    m_threadPool = app->getThreadPool();
}

ScanFileResult AssetScanner::scan(const std::filesystem::path& rootPath)
{
    auto tTotal0 = std::chrono::high_resolution_clock::now();

    ScanStats totalStats{};
    std::mutex statsMutex;

    std::vector<std::filesystem::path> files;

    auto tCollect0 = std::chrono::high_resolution_clock::now();
    collectFiles(rootPath, files);
    auto tCollect1 = std::chrono::high_resolution_clock::now();

    DEBUG_ASSETS("[AssetScanner] Collect files took %.3f ms", elapsedMs(tCollect0, tCollect1));
    DEBUG_ASSETS("[AssetScanner] Files collected: %zu", files.size());

    if (files.empty())
    {
        DEBUG_ASSETS("[AssetScanner] Empty scan.");
        return {};
    }

    const size_t numThreads = std::max<size_t>(1, m_threadPool->getNumThreads());
    const size_t maxIOThreads = std::max<size_t>(1, std::min<size_t>(numThreads / 2, 6));
    const size_t taskCount = std::min(files.size(), maxIOThreads);
    const size_t filesPerTask = (files.size() + taskCount - 1) / taskCount;

    DEBUG_ASSETS(
        "[AssetScanner] Threads: %zu | Max IO threads: %zu | IO tasks: %zu | Files per task: %zu",
        numThreads, maxIOThreads, taskCount, filesPerTask
    );

    std::vector<ScanFileResult> results(taskCount);
    std::vector<std::future<void>> futures;
    futures.reserve(taskCount);

    auto tParallel0 = std::chrono::high_resolution_clock::now();

    for (size_t taskIndex = 0; taskIndex < taskCount; ++taskIndex)
    {
        const size_t begin = taskIndex * filesPerTask;
        const size_t end = std::min(begin + filesPerTask, files.size());

        futures.push_back(m_threadPool->submitTask(
            [this, &files, &results, &totalStats, &statsMutex, taskIndex, begin, end]()
            {
                g_threadStats = {};

                auto tTask0 = std::chrono::high_resolution_clock::now();

                ScanFileResult& result = results[taskIndex];

                for (size_t i = begin; i < end; ++i)
                {
                    checkFile(files[i], result);
                }

                auto tTask1 = std::chrono::high_resolution_clock::now();

                {
                    std::lock_guard<std::mutex> lock(statsMutex);
                    totalStats.numFiles += g_threadStats.numFiles;
                    totalStats.numMetadata += g_threadStats.numMetadata;
                    totalStats.numMD5 += g_threadStats.numMD5;
                    totalStats.numMissingMetadata += g_threadStats.numMissingMetadata;
                    totalStats.numImportsQueued += g_threadStats.numImportsQueued;
                }

                DEBUG_ASSETS(
                    "[AssetScanner] Task %zu scanned %zu files in %.3f ms | metadata=%zu | md5=%zu | missingMeta=%zu | imports=%zu",
                    taskIndex, g_threadStats.numFiles, elapsedMs(tTask0, tTask1),
                    g_threadStats.numMetadata, g_threadStats.numMD5,
                    g_threadStats.numMissingMetadata, g_threadStats.numImportsQueued
                );
            }));
    }

    for (std::future<void>& future : futures)
    {
        future.get();
    }

    auto tParallel1 = std::chrono::high_resolution_clock::now();

    DEBUG_ASSETS("[AssetScanner] Parallel scan took %.3f ms", elapsedMs(tParallel0, tParallel1));

    ScanFileResult out;
    auto tMerge0 = std::chrono::high_resolution_clock::now();

    for (ScanFileResult& result : results)
    {
        for (Metadata& meta : result.metadata)
        {
            out.metadata.push_back(std::move(meta));
        }

        for (const ImportRequest& req : result.imports)
        {
            queueImport(out.imports, req.sourcePath, req.existingUID);
        }
    }

    auto tMerge1 = std::chrono::high_resolution_clock::now();

    DEBUG_ASSETS("[AssetScanner] Merge took %.3f ms", elapsedMs(tMerge0, tMerge1));

    auto tTotal1 = std::chrono::high_resolution_clock::now();

    DEBUG_ASSETS("[AssetScanner] ===== Scan report =====");
    DEBUG_ASSETS("[AssetScanner] Total took %.3f ms", elapsedMs(tTotal0, tTotal1));
    DEBUG_ASSETS("[AssetScanner] Files: %zu", totalStats.numFiles);
    DEBUG_ASSETS("[AssetScanner] Metadata files: %zu", totalStats.numMetadata);
    DEBUG_ASSETS("[AssetScanner] MD5 computed: %zu", totalStats.numMD5);
    DEBUG_ASSETS("[AssetScanner] Missing metadata: %zu", totalStats.numMissingMetadata);
    DEBUG_ASSETS("[AssetScanner] Imports queued before merge: %zu", totalStats.numImportsQueued);
    DEBUG_ASSETS("[AssetScanner] Final pending imports: %zu", out.imports.size());
    DEBUG_ASSETS("[AssetScanner] =======================");

    return out;
}

void AssetScanner::collectFiles(const std::filesystem::path& path, std::vector<std::filesystem::path>& files) const
{
    if (FileIO::isDirectory(path))
    {
        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
            collectFiles(entry.path(), files);
        }
        return;
    }

    files.push_back(path);
}

void AssetScanner::checkFile(const std::filesystem::path& path, ScanFileResult& result) const
{
    ++g_threadStats.numFiles;

    if (path.extension() == METADATA_EXTENSION)
    {
        ++g_threadStats.numMetadata;
        loadMetadata(path, result);
        return;
    }

    std::filesystem::path metadataPath = path;
    metadataPath += METADATA_EXTENSION;

    if (!FileIO::exists(metadataPath))
    {
        ++g_threadStats.numMissingMetadata;
        handleMissingMetadata(path, result);
    }
}

void AssetScanner::loadMetadata(const std::filesystem::path& metadataPath, ScanFileResult& result) const
{
    const std::filesystem::path sourcePath = metadataPath.parent_path() / metadataPath.stem();

    if (!FileIO::exists(sourcePath))
    {
        return;
    }

    Metadata meta;

    auto tLoadMeta0 = std::chrono::high_resolution_clock::now();

    if (!app->getModuleAssets()->loadMetaFile(metadataPath, meta))
    {
        DEBUG_ERROR("[AssetScanner] Failed to load metadata '%s'.", metadataPath.string().c_str());
        return;
    }

    auto tLoadMeta1 = std::chrono::high_resolution_clock::now();
    const double loadMetaMs = elapsedMs(tLoadMeta0, tLoadMeta1);

    if (loadMetaMs > 2.0)
    {
        DEBUG_ASSETS("[AssetScanner][Slow loadMetaFile] %.3f ms | %s", loadMetaMs, metadataPath.string().c_str());
    }

    meta.sourcePath = sourcePath.lexically_normal();
    result.metadata.push_back(meta);

    auto tBinaryExists0 = std::chrono::high_resolution_clock::now();
    const bool binaryMissing = !FileIO::exists(meta.getBinaryPath());
    auto tBinaryExists1 = std::chrono::high_resolution_clock::now();

    const double binaryExistsMs = elapsedMs(tBinaryExists0, tBinaryExists1);
    if (binaryExistsMs > 2.0)
    {
        DEBUG_ASSETS("[AssetScanner][Slow binary exists] %.3f ms | %s", binaryExistsMs, meta.getBinaryPath().string().c_str());
    }

    bool contentChanged = !isValidAsset(meta.contentHash);
    if (!contentChanged && hasSourceChanged(sourcePath, meta))
    {
        auto tMD50 = std::chrono::high_resolution_clock::now();
        const MD5Hash currentHash = computeMD5(sourcePath);
        auto tMD51 = std::chrono::high_resolution_clock::now();
        ++g_threadStats.numMD5;

        const double md5Ms = elapsedMs(tMD50, tMD51);
        if (md5Ms > 5.0)
        {
            DEBUG_ASSETS("[AssetScanner][Slow MD5] %.3f ms | %s", md5Ms, sourcePath.string().c_str());
        }

        contentChanged = (meta.contentHash != currentHash);
    }

    if (contentChanged || binaryMissing)
    {
        queueImport(result.imports, sourcePath, meta.uid);
    }

    for (const DependencyRecord& dep : meta.m_dependencies)
    {
        if (!isValidUID(dep.uid))
        {
            continue;
        }

        Metadata subMeta;
        subMeta.uid = dep.uid;
        subMeta.contentHash = dep.contentHash;
        subMeta.type = dep.type;
        subMeta.m_isSubAsset = true;

        result.metadata.push_back(subMeta);

        auto tDepExists0 = std::chrono::high_resolution_clock::now();
        const bool depBinaryMissing = !FileIO::exists(dep.getBinaryPath());
        auto tDepExists1 = std::chrono::high_resolution_clock::now();

        const double depExistsMs = elapsedMs(tDepExists0, tDepExists1);
        if (depExistsMs > 2.0)
        {
            DEBUG_ASSETS("[AssetScanner][Slow dep binary exists] %.3f ms | %s", depExistsMs, dep.getBinaryPath().string().c_str());
        }

        if (depBinaryMissing)
        {
            queueImport(result.imports, sourcePath, meta.uid);
        }
    }
}

void AssetScanner::handleMissingMetadata(const std::filesystem::path& sourcePath, ScanFileResult& result) const
{
    auto tFindImporter0 = std::chrono::high_resolution_clock::now();

    const bool hasImporter = app->getModuleAssets()->findImporter(sourcePath) != nullptr;

    auto tFindImporter1 = std::chrono::high_resolution_clock::now();

    const double findImporterMs = elapsedMs(tFindImporter0, tFindImporter1);
    if (findImporterMs > 2.0)
    {
        DEBUG_ASSETS("[AssetScanner][Slow findImporter] %.3f ms | %s", findImporterMs, sourcePath.string().c_str());
    }

    if (hasImporter)
    {
        queueImport(result.imports, sourcePath, INVALID_UID);
    }
}

bool AssetScanner::hasSourceChanged(const std::filesystem::path& sourcePath, const Metadata& meta) const
{
    if (meta.sourceFileSize == 0 && meta.sourceLastModified == 0)
        return true;

    std::error_code ec;

    const auto fileSize = std::filesystem::file_size(sourcePath, ec);
    if (ec)
    {
        DEBUG_WARN("[AssetScanner] Could not stat '%s': %s", sourcePath.string().c_str(), ec.message().c_str());
        return true;
    }

    if (static_cast<uint64_t>(fileSize) != meta.sourceFileSize)
    {
        return true;
    }

    const auto ftime = std::filesystem::last_write_time(sourcePath, ec);
    if (ec)
    {
        DEBUG_WARN("[AssetScanner] Could not read mtime of '%s': %s", sourcePath.string().c_str(), ec.message().c_str());
        return true;
    }

    const int64_t lastModified = static_cast<int64_t>(ftime.time_since_epoch().count());
    return lastModified != meta.sourceLastModified;
}

void AssetScanner::queueImport(
    std::vector<ImportRequest>& imports,
    const std::filesystem::path& sourcePath,
    const UID& existingUID)
{
    for (const ImportRequest& req : imports)
    {
        if (req.sourcePath == sourcePath)
        {
            return;
        }
    }

    imports.push_back({ sourcePath, existingUID });
    ++g_threadStats.numImportsQueued;
}