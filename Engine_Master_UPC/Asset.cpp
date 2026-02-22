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
        LOG_ERROR("[AssetsModule] Couldn't open the metafile.");
        return false;
    }

    file << buffer.GetString();
    return true;
}

bool AssetMetadata::loadMetaFile(const std::filesystem::path& metaPath, AssetMetadata& outMeta)
{
    try
    {
        simdjson::ondemand::parser parser;
        simdjson::padded_string json = simdjson::padded_string::load(metaPath.string());

        auto doc = parser.iterate(json);

        outMeta.uid = doc["uid"].get_uint64().value();
        outMeta.type = static_cast<AssetType>(doc["type"].get_uint64().value());

        outMeta.sourcePath = std::string(doc["source"].get_string().value());
        outMeta.binaryPath = std::string(doc["binary"].get_string().value());

        return true;
    }
    catch (const simdjson::simdjson_error& e)
    {
        LOG_ERROR("[FileSystemModule] Failed to load meta file '%s': %s", metaPath.string().c_str(), e.what());
        return false;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("[FileSystemModule] Unexpected error loading meta file '%s': %s", metaPath.string().c_str(), e.what());
        return false;
    }
}
