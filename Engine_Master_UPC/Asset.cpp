#include "Globals.h"
#include "Asset.h"

bool AssetMetadata::saveMetaFile(const AssetMetadata& meta, const std::filesystem::path& metaPath)
{
    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    // uid — MD5Hash is a std::string
    doc.AddMember("uid", rapidjson::Value(meta.uid.c_str(), alloc), alloc);

    // type — store as uint32 so IsUint() / GetUint() round-trips cleanly.
    // (Storing as signed int and loading with IsUint64() was the original bug:
    //  RapidJSON's IsUint64() returns false for values written as signed int.)
    doc.AddMember("type", rapidjson::Value(static_cast<uint32_t>(meta.type)), alloc);

    // sourcePath — store as a portable relative string so the registry can
    // reconstruct it on load without relying on caller-side fixups.
    doc.AddMember("sourcePath", rapidjson::Value(meta.sourcePath.string().c_str(), alloc), alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream file(metaPath);
    if (!file.is_open())
    {
        DEBUG_ERROR("[AssetMetadata] Could not open '%s' for writing.", metaPath.string().c_str());
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

bool AssetMetadata::loadMetaFile(const std::filesystem::path& metaPath, AssetMetadata& outMeta)
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

    // uid
    if (doc.HasMember("uid") && doc["uid"].IsString())
    {
        outMeta.uid = doc["uid"].GetString();
    }
    else
    {
        DEBUG_ERROR("[AssetMetadata] Missing or invalid 'uid' in '%s'.", pathStr.c_str());
        return false;
    }

    // type — accept both uint and int to stay compatible with older files that
    // were written with static_cast<int>.
    if (doc.HasMember("type") && doc["type"].IsNumber())
    {
        outMeta.type = static_cast<AssetType>(doc["type"].GetUint());
    }
    else
    {
        DEBUG_ERROR("[AssetMetadata] Missing or invalid 'type' in '%s'.", pathStr.c_str());
        return false;
    }

    // sourcePath — optional: if absent the caller is responsible for filling it.
    if (doc.HasMember("sourcePath") && doc["sourcePath"].IsString())
    {
        outMeta.sourcePath = doc["sourcePath"].GetString();
    }

    return true;
}