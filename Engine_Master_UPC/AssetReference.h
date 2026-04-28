#pragma once
#include "UID.h"
#include <cereal/access.hpp>

struct AssetReference {
    friend class cereal::access;

    AssetReference() = default;
    AssetReference(UID fileId, UID localId = INVALID_UID): fileId(fileId), localId(localId) {}

	UID fileId = INVALID_UID;
	UID localId = INVALID_UID;

	bool isValid()    const { return isValidUID(fileId); }
	bool isSubAsset() const { return isValidUID(fileId) && isValidUID(localId); }

	bool operator==(const AssetReference& o) const
	{
		return fileId == o.fileId && localId == o.localId;
	}

	bool operator!=(const AssetReference& o) const { return !(*this == o); }

    rapidjson::Value getJson(rapidjson::Document::AllocatorType& allocator) const;
    bool deserializeJson(const rapidjson::Value& obj);

	template <class Archive>
	void serialize(Archive& ar)
	{
		ar(fileId, localId);
	}
};

inline AssetReference makeRef(UID fileId) { return { fileId, INVALID_UID }; }
inline AssetReference makeRef(UID fileId, UID localId) { return { fileId, localId }; }

namespace std
{
    template<>
    struct hash<AssetReference>
    {
        std::size_t operator()(const AssetReference& ref) const noexcept
        {
            std::size_t h1 = std::hash<UID>{}(ref.fileId);
            std::size_t h2 = std::hash<UID>{}(ref.localId);
            return h1 ^ (h2 << 32 | h2 >> 32);
        }
    };
}