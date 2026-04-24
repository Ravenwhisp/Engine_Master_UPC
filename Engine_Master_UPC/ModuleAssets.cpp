#include "Globals.h"
#include "ModuleAssets.h"

#include "Application.h"
#include "Importer.h"
#include "ImporterMesh.h"
#include "ImporterMaterial.h"
#include "ImporterTexture.h"
#include "ImporterPrefab.h"
#include "ImporterAnimation.h"
#include "ImporterSkin.h"
#include "ImporterAnimationStateMachine.h"
#include "ImporterGltf.h"
#include "ImporterFont.h"
#include "MD5.h"

#include "Asset.h"
#include "AnimationStateMachineAsset.h"
#include "Metadata.h"
#include "UID.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include "rapidjson/filereadstream.h"
#include <fstream>

#include <filesystem>
#include <FileIO.h>
#include <AssetDialogFilter.h>
#include <FileDialog.h>
#include "FileDialogRequest.h"

ModuleAssets::~ModuleAssets() = default;

bool ModuleAssets::init()
{
    m_registry = std::make_unique<AssetRegistry>();
    
	m_importers.push_back(m_importerTexture = new ImporterTexture());
    m_importers.push_back(m_importerMesh = new ImporterMesh());
    m_importers.push_back(m_importerMaterial = new ImporterMaterial());
    m_importers.push_back(m_importerPrefab = new ImporterPrefab());
    m_importers.push_back(m_importerAnimation = new ImporterAnimation());
    m_importers.push_back(m_importerSkin = new ImporterSkin());
    m_importers.push_back(m_importerAnimationStateMachine = new ImporterAnimationStateMachine());
    m_importers.push_back(m_importerGltf = new ImporterGltf(m_importerMesh, m_importerMaterial, m_importerPrefab, m_importerAnimation, m_importerSkin, m_importerAnimationStateMachine));
    m_importers.push_back(m_importerFont = new ImporterFont());

    m_scanner = std::make_unique<AssetScanner>(m_registry.get());
    m_contentRegistry = std::make_unique<ContentRegistry>(m_registry.get());

    refresh();

    return true;
}

bool ModuleAssets::cleanUp()
{
    m_assets.clear();
    for (auto it = m_importers.rbegin(); it != m_importers.rend(); ++it)
    {
        delete* it;
    }

    return true;
}

bool ModuleAssets::canImport(const std::filesystem::path& sourcePath) const
{
    return findImporter(sourcePath) != nullptr;
}

void ModuleAssets::importAsset(const std::filesystem::path& sourcePath, MD5Hash& uid)
{
    Importer* importer = findImporter(sourcePath);
    if (!importer)
    {
        DEBUG_WARN("[ModuleAssets] No importer found for '%s'.", sourcePath.string().c_str());
        uid = INVALID_ASSET_ID;
        return;
    }

    if (!isValidAsset(uid))
    {
        uid = computeMD5(sourcePath);
    }

    // Scoped to this function — not cached, not shared.
    std::unique_ptr<Asset> asset(importer->createAssetInstance(uid));

    if (!importer->import(sourcePath, asset.get()))
    {
        DEBUG_ERROR("[ModuleAssets] Import failed for '%s'.", sourcePath.string().c_str());
        uid = INVALID_ASSET_ID;
        return;
    }

    // Write the .metadata sidecar alongside the source file.
    Metadata meta;
    meta.uid = uid;
    meta.type = asset->getAssetType();
    meta.sourcePath = sourcePath;
    
    auto it = m_pendingDependencies.find(uid);
    if (it != m_pendingDependencies.end()) {
        meta.m_dependencies = std::move(it->second);
        m_pendingDependencies.erase(it);
    }

    std::filesystem::path metaPath = sourcePath;
    metaPath += METADATA_EXTENSION;

    if (!save(meta, metaPath))
    {
        DEBUG_ERROR("[ModuleAssets] Failed to write metadata for '%s'.", sourcePath.string().c_str());
        uid = INVALID_ASSET_ID;
        return;
    }

    m_registry->registerAsset(meta);

    // Serialise the processed binary into the library folder.
    uint8_t* rawBuffer = nullptr;
    const uint64_t size = importer->save(asset.get(), &rawBuffer);
    std::unique_ptr<uint8_t[]> buffer(rawBuffer);

    if (!FileIO::write(meta.getBinaryPath(), buffer.get(), static_cast<size_t>(size)))
    {
        DEBUG_ERROR("[ModuleAssets] Failed to write binary for '%s'.", sourcePath.string().c_str());
        m_registry->remove(uid);
        uid = INVALID_ASSET_ID;
        return;
    }

    return;
}

void ModuleAssets::refresh()
{
    std::string rootStr = ASSETS_FOLDER;
    if (!rootStr.empty() && (rootStr.back() == '/' || rootStr.back() == '\\'))
    {
        rootStr.pop_back();
    }

    const std::filesystem::path root = rootStr;

    std::vector<ImportRequest> pending = m_scanner->scan(root);
    for (ImportRequest& req : pending)
    {
        importAsset(req.sourcePath, req.existingUID);
    }

    m_contentRegistry->rebuild(root);
}


MD5Hash ModuleAssets::findUID(const std::filesystem::path& sourcePath) const
{
    return m_registry->findByPath(sourcePath);
}

bool ModuleAssets::isLoaded(const MD5Hash& id)
{
    return m_assets.contains(id);
}

void ModuleAssets::unload(const MD5Hash& id)
{
    m_assets.remove(id);
}

std::shared_ptr<FileEntry> ModuleAssets::getRoot() const
{
    return m_contentRegistry->getRoot();
}

std::shared_ptr<FileEntry> ModuleAssets::getEntry(const std::filesystem::path& path) const
{
    return m_contentRegistry->getEntry(path);
}


std::shared_ptr<Asset> ModuleAssets::loadAsset(const Metadata* metadata)
{
    Importer* importer = findImporter(metadata->type);
    if (!importer)
    {
        DEBUG_ERROR("[ModuleAssets] No importer for asset type %u (UID %llu).", static_cast<unsigned>(metadata->type), metadata->uid);
        return nullptr;
    }

    std::shared_ptr<Asset> asset(importer->createAssetInstance(metadata->uid));

    const std::vector<uint8_t> buffer = FileIO::read(metadata->getBinaryPath());
    if (buffer.empty())
    {
        DEBUG_ERROR("[ModuleAssets] Binary missing or empty for UID %llu.", metadata->uid);
        return nullptr;
    }

    importer->load(buffer.data(), asset.get());
    m_assets.insert(metadata->uid, asset);

    return asset;
}

bool ModuleAssets::save(const ISerializable& obj, const std::filesystem::path& path)
{
    std::filesystem::path targetPath = path;

    if (targetPath.empty())
    {
        const AssetDialogFilter filter = getDialogFilter(obj.getAssetType());

        auto chosen = saveAs(filter.filterSpec, filter.defaultExtension, "Save Asset", ASSETS_FOLDER);

        if (!chosen)
        {
            return false;
        }

        targetPath = std::move(*chosen);
    }

    rapidjson::Document doc;
    doc.SetObject();

    if (!obj.toJson(doc))
    {
        DEBUG_ERROR("[ModuleAssets] toJson() failed for '%s'.", targetPath.string().c_str());
        return false;
    }

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream file(targetPath);
    if (!file.is_open())
    {
        DEBUG_ERROR("[ModuleAssets] Could not open '%s' for writing.", targetPath.string().c_str());
        return false;
    }

    file << buffer.GetString();
    if (!file)
    {
        DEBUG_ERROR("[ModuleAssets] Write failed for '%s'.", targetPath.string().c_str());
        return false;
    }

    return true;
}

bool ModuleAssets::load(const std::filesystem::path& path, ISerializable& obj)
{
    const std::string pathStr = path.string();
    FILE* fp = std::fopen(pathStr.c_str(), "rb");
    if (!fp)
    {
        DEBUG_ERROR("[JsonFile] Could not open '%s'.", pathStr.c_str());
        return false;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document doc;
    doc.ParseStream(is);
    std::fclose(fp);

    if (doc.HasParseError())
    {
        DEBUG_ERROR("[JsonFile] JSON parse error in '%s'.", pathStr.c_str());
        return false;
    }

    if (!obj.fromJson(doc))
    {
        DEBUG_ERROR("[JsonFile] fromJson() failed for '%s'.", pathStr.c_str());
        return false;
    }

    return true;
}


void ModuleAssets::registerSubAsset(const Metadata& meta,
    const MD5Hash& parentUID,
    uint8_t* binaryData, size_t binarySize)
{
    Metadata subMeta = meta;
    subMeta.m_isSubAsset = true;

    // Write the binary to Library/ — caller still owns the buffer.
    if (binaryData && binarySize > 0)
    {
        if (!FileIO::write(subMeta.getBinaryPath(), binaryData, binarySize))
        {
            DEBUG_ERROR("[ModuleAssets] Failed to write sub-asset binary '%s'.", subMeta.uid.c_str());
            return;
        }
    }

    m_registry->registerAsset(subMeta);

    if (isValidAsset(parentUID))
    {
        DependencyRecord dep;
        dep.uid = subMeta.uid;
        dep.type = subMeta.type;
        m_pendingDependencies[parentUID].push_back(dep);
    }
}

#pragma region Importer

Importer* ModuleAssets::findImporter(const std::filesystem::path& filePath) const
{
    for (auto& importer : m_importers)
    {
        if (importer->canImport(filePath))
        {
            return importer;
        }
    }
    return nullptr;
}

Importer* ModuleAssets::findImporter(AssetType type) const
{
    for (auto& importer : m_importers)
    {
        if (importer->getAssetType() == type)
        {
            return importer;
        }
    }
    return nullptr;
}

#pragma endregion

void ModuleAssets::requestSave(const ISerializable& obj)
{
    if (m_dialogRunning.load())
        return;   // one dialog at a time

    m_pendingSerializable = &obj;
    m_pendingAssetType = obj.getAssetType();
    m_pendingIsSave = true;
    m_dialogCallback = nullptr;

    // Clear any previous result
    {
        std::lock_guard lock(m_dialogResultMutex);
        m_dialogResult.reset();
    }

    m_dialogRunning.store(true);

    // Spin up a worker — main thread is free to keep rendering
    std::thread([this]()
        {
            const AssetDialogFilter filter = getDialogFilter(m_pendingAssetType);

            std::optional<std::filesystem::path> result =
                saveAs(filter.filterSpec,
                    filter.defaultExtension,
                    "Save Asset", ASSETS_FOLDER);

            // Hand result back to the main thread safely
            {
                std::lock_guard lock(m_dialogResultMutex);
                m_dialogResult = std::move(result);
            }

            m_dialogRunning.store(false);

        }).detach();
}

void ModuleAssets::requestOpen(AssetType type,
    std::function<void(const std::filesystem::path&)> onConfirm)
{
    if (m_dialogRunning.load())
        return;

    m_pendingSerializable = nullptr;
    m_pendingAssetType = type;
    m_pendingIsSave = false;
    m_dialogCallback = std::move(onConfirm);

    {
        std::lock_guard lock(m_dialogResultMutex);
        m_dialogResult.reset();
    }

    m_dialogRunning.store(true);

    std::thread([this]()
        {
            const AssetDialogFilter filter = getDialogFilter(m_pendingAssetType);

            std::optional<std::filesystem::path> result = open(filter.filterSpec, "Open Asset", ASSETS_FOLDER);

            {
                std::lock_guard lock(m_dialogResultMutex);
                m_dialogResult = std::move(result);
            }

            m_dialogRunning.store(false);

        }).detach();
}

void ModuleAssets::flushDialogRequests()
{
    // Still running — nothing to flush yet
    if (m_dialogRunning.load())
        return;

    std::optional<std::filesystem::path> result;
    {
        std::lock_guard lock(m_dialogResultMutex);
        if (!m_dialogResult.has_value())
            return;   // no pending result at all (dialog not yet requested)

        result = std::move(m_dialogResult);
        m_dialogResult.reset();
    }

    if (!result.has_value())
    {
        m_pendingSerializable = nullptr;
        return;
    }

    if (m_pendingIsSave && m_pendingSerializable)
    {
        save(*m_pendingSerializable , *result);
        m_pendingSerializable = nullptr;
    }
    else if (!m_pendingIsSave && m_dialogCallback)
    {
        m_dialogCallback(*result);
        m_dialogCallback = nullptr;
    }
}



void ModuleAssets::flushDependencies(const MD5Hash& parentUID,
    const std::filesystem::path& parentSourcePath,
    AssetType parentType)
{
    auto it = m_pendingDependencies.find(parentUID);
    if (it == m_pendingDependencies.end())
        return;

    // Upsert the parent's registry entry with the collected dependency list.
    Metadata parentMeta;
    const Metadata* existing = m_registry->getMetadata(parentUID);
    if (existing)
    {
        parentMeta = *existing;
    }
    else
    {
        parentMeta.uid = parentUID;
        parentMeta.type = parentType;
        parentMeta.sourcePath = parentSourcePath;
    }

    parentMeta.m_dependencies = std::move(it->second);
    m_pendingDependencies.erase(it);

    m_registry->registerAsset(parentMeta);
}