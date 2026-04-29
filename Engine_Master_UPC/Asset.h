#pragma once
#include "AssetsDictionary.h"
#include <AssetType.h>
#include "AssetReference.h"
#include "UID.h"

class Asset
{
public:
	friend class cereal::access;

	Asset() = default;
	Asset(UID id, AssetType type = AssetType::UNKNOWN): m_uid(id), m_type(type) {}
	virtual ~Asset() = default;
	UID getId() const { return m_uid; }
	AssetType getType() const { return m_type; }
protected:
	UID 				m_uid = INVALID_UID;
	AssetType			m_type = AssetType::UNKNOWN;
public:
	template <class Archive>
	void serialize(Archive& ar)
	{
		ar(m_uid, m_type);
	}
};
