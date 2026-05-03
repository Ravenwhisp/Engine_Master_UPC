#pragma once
#include "UID.h"
#include <MD5.h>

struct AssetReference {

    AssetReference() = default;
    AssetReference(UID uid, MD5Hash m_libId) : m_uid(uid), m_libId(m_libId) {}

    UID m_uid = INVALID_UID;
    MD5Hash m_libId = INVALID_ASSET_ID;

    bool isValid() const { return isValidUID(m_uid) && isValidAsset(m_libId); }

    bool operator==(const AssetReference& o) const
    {
        return m_uid == o.m_uid && m_libId == o.m_libId;
    }

    bool operator!=(const AssetReference& o) const { return !(*this == o); }

    rapidjson::Value getJson(rapidjson::Document::AllocatorType& allocator) const;
    bool deserializeJson(const rapidjson::Value& obj);


};
