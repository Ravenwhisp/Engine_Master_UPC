#pragma once

#include "MD5Fwd.h"
#include "AssetsDictionary.h"
#include "AssetType.h"

class Asset
{
public:
	Asset() = default;
	Asset(UID id, AssetType type = AssetType::UNKNOWN): m_uid(id), m_type(type) {}
	virtual ~Asset() = default;
	UID getId() const { return m_uid; }

	AssetType	getType() const { return m_type; }
protected:
	UID 				m_uid = INVALID_UID;
	AssetType			m_type = AssetType::UNKNOWN;
};
