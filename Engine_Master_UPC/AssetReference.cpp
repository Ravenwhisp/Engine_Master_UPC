#include "Globals.h"
#include "AssetReference.h"

rapidjson::Value AssetReference::getJson(rapidjson::Document::AllocatorType& allocator) const
{
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("fileId", fileId, allocator);
    obj.AddMember("localId", localId, allocator);
    return obj;
}

bool AssetReference::deserializeJson(const rapidjson::Value& obj)
{
    if (!obj.IsObject())
    {
        DEBUG_ERROR("[AssetReference] Expected a JSON object.");
        return false;
    }

    if (!obj.HasMember("fileId") || !obj["fileId"].IsUint64())
    {
        DEBUG_ERROR("[AssetReference] Missing or invalid 'fileId'.");
        return false;
    }

    fileId = obj["fileId"].GetUint64();
    localId = obj.HasMember("localId") && obj["localId"].IsUint64() ? obj["localId"].GetUint64() : INVALID_UID;

    return true;
}
