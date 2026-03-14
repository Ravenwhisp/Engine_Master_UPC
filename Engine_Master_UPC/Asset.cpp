#include "Globals.h"
#include "Asset.h"

bool AssetMetadata::saveMetaFile(const AssetMetadata& meta, const std::filesystem::path& metaPath)
{
    rapidjson::Document doc;
    doc.SetObject();

    auto& alloc = doc.GetAllocator();

    rapidjson::Value uidVal(meta.uid.c_str(), alloc);
    doc.AddMember("uid", uidVal, alloc);
    doc.AddMember("type", rapidjson::Value(static_cast<int>(meta.type)), alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream file(metaPath);
    if (!file.is_open())
    {
        DEBUG_LOG("[ModuleAssets] Couldn't open the metafile.");
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
        DEBUG_ERROR("[ModuleFileSystem] Failed to open meta file '%s'", metaPath.string().c_str());
        return false;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document doc;
    doc.ParseStream(is);
    std::fclose(fp);

    if (doc.HasParseError()) {
        DEBUG_ERROR("[ModuleFileSystem] JSON parse error in '%s'", metaPath.string().c_str());
        return false;
    }

    // Extract values safely
    if (doc.HasMember("uid") && doc["uid"].IsString())
    {
        outMeta.uid = doc["uid"].GetString();
    }
    else 
    {
        DEBUG_ERROR("[ModuleFileSystem] Missing or invalid 'uid' in '%s'", metaPath.string().c_str());
        return false;
    }

    if (doc.HasMember("type") && doc["type"].IsUint64()) 
    {
        outMeta.type = static_cast<AssetType>(doc["type"].GetUint64());
    }
    else {
        DEBUG_ERROR("[ModuleFileSystem] Missing or invalid 'type' in '%s'", metaPath.string().c_str());
        return false;
    }

    return true;
}
