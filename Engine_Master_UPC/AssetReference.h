#pragma once
#include "UID.h"
#include <MD5.h>
#include "AssetType.h"
#include "IArchive.h"

class ISerializable;

struct AssetReference {

    AssetReference() = default;
    explicit AssetReference(UID uid): m_uid(uid) {}

    AssetReference(UID uid, MD5Hash libId) : m_uid(uid), m_libId(std::move(libId)) {}

    AssetReference(UID uid, MD5Hash libId, AssetType type) : m_uid(uid), m_libId(std::move(libId)), m_type(type) {}


    UID m_uid = INVALID_UID;
    MD5Hash m_libId = INVALID_ASSET_ID;
    AssetType m_type = AssetType::UNKNOWN;

    bool hasUID()  const { return isValidUID(m_uid); }
    bool isValid() const { return isValidUID(m_uid) && isValidAsset(m_libId); }

    bool operator==(const AssetReference& o) const
    {
        return m_uid == o.m_uid && m_libId == o.m_libId;
    }

    bool operator!=(const AssetReference& o) const { return !(*this == o); }

    void serialize(IArchive& archive);

};
