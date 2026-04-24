#pragma once
#include "AssetsDictionary.h"
#include <AssetType.h>
#include "MD5Fwd.h"
#include <cereal/types/base_class.hpp>

class Asset
{
public:
	friend class cereal::access;

	Asset() = default;
	Asset(MD5Hash id, AssetType type = AssetType::UNKNOWN): m_uid(id), m_type(type) {}
	virtual ~Asset() = default;
	MD5Hash getId() const { return m_uid; }
	AssetType getType() const { return m_type; }
protected:
	MD5Hash 			m_uid = INVALID_ASSET_ID;
	AssetType			m_type = AssetType::UNKNOWN;
private:
	template <class Archive>
	void serialize(Archive& ar)
	{
		ar(m_uid, m_type);
	}
};
