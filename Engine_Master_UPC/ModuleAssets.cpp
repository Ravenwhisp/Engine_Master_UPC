#include "Globals.h"
#include "ModuleAssets.h"

#include "Application.h"
#include "ModuleFileSystem.h"
#include "Importer.h"
#include "ImporterMesh.h"
#include "ImporterMaterial.h"
#include "ImporterTexture.h"
#include "ImporterPrefab.h"
#include "ImporterAnimation.h"
#include "ImporterGltf.h"
#include "ImporterFont.h"
#include "MD5.h"

#include "Asset.h"
#include "Metadata.h"
#include "UID.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include "rapidjson/filereadstream.h"

#include <filesystem>


bool ModuleAssets::init()
{
    m_registry = std::make_unique<AssetRegistry>();
    m_importerRegistry = std::make_unique<ImporterRegistry>();
    
    m_importerRegistry->registerImporter(std::make_unique<ImporterTexture>());

    {
        auto mesh = std::make_unique<ImporterMesh>();
        m_importerMesh = mesh.get();
        m_importerRegistry->registerImporter(std::move(mesh));
    }
    {
        auto mat = std::make_unique<ImporterMaterial>();
        m_importerMaterial = mat.get();
        m_importerRegistry->registerImporter(std::move(mat));
    }
    {
        auto prefab = std::make_unique<ImporterPrefab>();
        m_importerPrefab = prefab.get();
        m_importerRegistry->registerImporter(std::move(prefab));
    }

    {
        auto anim = std::make_unique<ImporterAnimation>();
        m_importerAnimation = anim.get();
        m_importerRegistry->registerImporter(std::move(anim));
    }

    // GLTF importer holds references to the three importers above so it can
    // delegate sub-asset serialisation without duplicating binary format logic.
    m_importerRegistry->registerImporter(
        std::make_unique<ImporterGltf>(*m_importerMesh, *m_importerMaterial, *m_importerPrefab));

    m_importerRegistry->registerImporter(std::make_unique<ImporterFont>());

    ModuleFileSystem* fs = app->getModuleFileSystem();
    m_scanner = std::make_unique<AssetScanner>(fs, m_registry.get(), m_importerRegistry.get());
    m_contentRegistry = std::make_unique<ContentRegistry>(fs, m_registry.get());

    refresh();

    return true;
}

bool ModuleAssets::cleanUp()
{
    m_assets.clear();
    return true;
}

bool ModuleAssets::canImport(const std::filesystem::path& sourcePath) const
{
    return m_importerRegistry->findImporter(sourcePath) != nullptr;
}

void ModuleAssets::importAsset(const std::filesystem::path& sourcePath, MD5Hash& uid)
{
    Importer* importer = m_importerRegistry->findImporter(sourcePath);
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
    meta.type = asset->getType();
    meta.sourcePath = sourcePath;
    
    auto it = m_pendingDependencies.find(uid);
    if (it != m_pendingDependencies.end()) {
        meta.m_dependencies = std::move(it->second);
        m_pendingDependencies.erase(it);
    }

    std::filesystem::path metaPath = sourcePath;
    metaPath += METADATA_EXTENSION;

    if (!saveMetaFile(meta, metaPath))
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

    if (!app->getModuleFileSystem()->write(meta.getBinaryPath(), buffer.get(), static_cast<size_t>(size)))
    {
        DEBUG_ERROR("[ModuleAssets] Failed to write binary for '%s'.", sourcePath.string().c_str());
        // Metadata was already written — roll back the in-memory store entry
        // so it doesn't reference a binary that doesn't exist.
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
    Importer* importer = m_importerRegistry->findImporter(metadata->type);
    if (!importer)
    {
        DEBUG_ERROR("[ModuleAssets] No importer for asset type %u (UID %llu).", static_cast<unsigned>(metadata->type), metadata->uid);
        return nullptr;
    }

    std::shared_ptr<Asset> asset(importer->createAssetInstance(metadata->uid));

    const std::vector<uint8_t> buffer = app->getModuleFileSystem()->read(metadata->getBinaryPath());
    if (buffer.empty())
    {
        DEBUG_ERROR("[ModuleAssets] Binary missing or empty for UID %llu.", metadata->uid);
        return nullptr;
    }

    importer->load(buffer.data(), asset.get());
    m_assets.insert(metadata->uid, asset);

    return asset;
}

bool ModuleAssets::saveMetaFile(const Metadata& meta,
    const std::filesystem::path& metaPath)
{
    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("uid", rapidjson::Value(meta.uid.c_str(), alloc), alloc);
    doc.AddMember("type", rapidjson::Value(static_cast<uint32_t>(meta.type)), alloc);
    doc.AddMember("sourcePath", rapidjson::Value(meta.sourcePath.string().c_str(), alloc), alloc);

    // Write the dependency list so the scanner can re-register sub-assets on
    // the next startup without re-importing the parent source file.
    if (!meta.m_dependencies.empty())
    {
        rapidjson::Value deps(rapidjson::kArrayType);
        for (const DependencyRecord& dep : meta.m_dependencies)
        {
            rapidjson::Value entry(rapidjson::kObjectType);
            entry.AddMember("uid", rapidjson::Value(dep.uid.c_str(), alloc), alloc);
            entry.AddMember("type", rapidjson::Value(static_cast<uint32_t>(dep.type)), alloc);
            deps.PushBack(entry, alloc);
        }
        doc.AddMember("dependencies", deps, alloc);
    }

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream file(metaPath);
    if (!file.is_open())
    {
        DEBUG_ERROR("[AssetMetadata] Could not open '%s' for writing.",
            metaPath.string().c_str());
        return false;
    }

    file << buffer.GetString();
    if (!file)
    {
        DEBUG_ERROR("[AssetMetadata] Failed to write '%s'.", metaPath.string().c_str());
        return false;
    }

    return true;
}

bool ModuleAssets::loadMetaFile(const std::filesystem::path& metaPath, Metadata& outMeta)
{
    const std::string pathStr = metaPath.string();
    FILE* fp = std::fopen(pathStr.c_str(), "rb");
    if (!fp)
    {
        DEBUG_ERROR("[AssetMetadata] Could not open '%s'.", pathStr.c_str());
        return false;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document doc;
    doc.ParseStream(is);
    std::fclose(fp);

    if (doc.HasParseError())
    {
        DEBUG_ERROR("[AssetMetadata] JSON parse error in '%s'.", pathStr.c_str());
        return false;
    }

    if (doc.HasMember("uid") && doc["uid"].IsString())
    {
        outMeta.uid = doc["uid"].GetString();
    }
    else
    {
        DEBUG_ERROR("[AssetMetadata] Missing 'uid' in '%s'.", pathStr.c_str());
        return false;
    }

    if (doc.HasMember("type") && doc["type"].IsNumber())
    {
        outMeta.type = static_cast<AssetType>(doc["type"].GetUint());
    }
    else
    {
        DEBUG_ERROR("[AssetMetadata] Missing 'type' in '%s'.", pathStr.c_str());
        return false;
    }

    if (doc.HasMember("sourcePath") && doc["sourcePath"].IsString())
    {
        outMeta.sourcePath = doc["sourcePath"].GetString();
    }


    // Restore the dependency list.
    outMeta.m_dependencies.clear();
    if (doc.HasMember("dependencies") && doc["dependencies"].IsArray())
    {
        const auto& deps = doc["dependencies"];
        outMeta.m_dependencies.reserve(deps.Size());
        for (rapidjson::SizeType i = 0; i < deps.Size(); ++i)
        {
            const auto& entry = deps[i];
            if (!entry.HasMember("uid") || !entry["uid"].IsString())  continue;
            if (!entry.HasMember("type") || !entry["type"].IsNumber()) continue;

            DependencyRecord rec;
            rec.uid = entry["uid"].GetString();
            rec.type = static_cast<AssetType>(entry["type"].GetUint());
            outMeta.m_dependencies.push_back(std::move(rec));
        }
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
        if (!app->getModuleFileSystem()->write(subMeta.getBinaryPath(), binaryData, binarySize))
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
        parentMeta = *existing;
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