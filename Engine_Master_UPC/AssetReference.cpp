#include "Globals.h"
#include "AssetReference.h"

rapidjson::Value AssetReference::getJson(rapidjson::Document::AllocatorType& allocator) const
{
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("uid", m_uid, allocator);
    obj.AddMember("libId", rapidjson::Value(m_libId.c_str(), allocator), allocator);
    return obj;
}

void AssetReference::serialize(IArchive& archive)
{
    archive.serialize(m_uid, "uid");
    archive.serialize(m_libId, "libId");
    archive.serializeStringEnum(m_type, "type", AssetTypeToString, StringToAssetType);
}

bool AssetReference::deserializeJson(const rapidjson::Value& obj)
{
    if (!obj.IsObject())
    {
        DEBUG_ERROR("[AssetReference] Expected a JSON object.");
        return false;
    }

    if (!obj.HasMember("uid") || !obj["uid"].IsUint64())
    {
        DEBUG_ERROR("[AssetReference] Missing or invalid 'fileId'.");
        return false;
    }

    m_uid = obj["uid"].GetUint64();
    m_libId = obj.HasMember("libId") && obj["libId"].IsString() ? obj["libId"].GetString() : INVALID_ASSET_ID;

    return true;
}

