#include "Globals.h"
#include "Asset.h"

bool AssetMetadata::saveMetaFile(const AssetMetadata& meta, const std::filesystem::path& metaPath)
{
    rapidjson::Document doc;
    doc.SetObject();

    auto& alloc = doc.GetAllocator();

    doc.AddMember("uid", rapidjson::Value(meta.uid), alloc);
    doc.AddMember("type", rapidjson::Value(static_cast<int>(meta.type)), alloc);
    doc.AddMember("source", rapidjson::Value(meta.sourcePath.string().c_str(), alloc), alloc);
    doc.AddMember("binary", rapidjson::Value(meta.binaryPath.string().c_str(), alloc), alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream file(metaPath);
    if (!file.is_open())
    {
        LOG("[AssetsModule] Couldn't open the metafile.");
        return false;
    }

    file << buffer.GetString();
    return true;
}

bool AssetMetadata::loadMetaFile(const std::filesystem::path& metaPath, AssetMetadata& outMeta)
{
    std::string pathStr = metaPath.string();
    const char* cpath = pathStr.c_str();
    FILE* fp = std::fopen(cpath, "rb");

    if (!fp) 
    {
        LOG_ERROR("[FileSystemModule] Failed to open meta file '%s'", metaPath.string().c_str());
        return false;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document doc;
    doc.ParseStream(is);
    std::fclose(fp);

    if (doc.HasParseError()) {
        LOG("[FileSystemModule] JSON parse error in '%s'", metaPath.string().c_str());
        return false;
    }

    // Extract values safely
    if (doc.HasMember("uid") && doc["uid"].IsUint64()) 
    {
        outMeta.uid = doc["uid"].GetUint64();
    }
    else 
    {
        LOG("[FileSystemModule] Missing or invalid 'uid' in '%s'", metaPath.string().c_str());
        return false;
    }

    if (doc.HasMember("type") && doc["type"].IsUint64()) 
    {
        outMeta.type = static_cast<AssetType>(doc["type"].GetUint64());
    }
    else {
        LOG("[FileSystemModule] Missing or invalid 'type' in '%s'", metaPath.string().c_str());
        return false;
    }

    if (doc.HasMember("source") && doc["source"].IsString()) 
    {
        outMeta.sourcePath = doc["source"].GetString();
    }
    else {
        LOG("[FileSystemModule] Missing or invalid 'source' in '%s'", metaPath.string().c_str());
        return false;
    }

    if (doc.HasMember("binary") && doc["binary"].IsString()) 
    {
        outMeta.binaryPath = doc["binary"].GetString();
    }
    else 
    {
        LOG("[FileSystemModule] Missing or invalid 'binary' in '%s'", metaPath.string().c_str());
        return false;
    }

    return true;
}
