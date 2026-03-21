#pragma once

#include "MD5Fwd.h"
#include "AssetsDictionary.h"
#include "AssetType.h"

class Asset
{
public:
	Asset() = default;
	Asset(MD5Hash id, AssetType type = AssetType::UNKNOWN): m_uid(id), m_type(type) {}
	virtual ~Asset() = default;
	MD5Hash getId() const { return m_uid; }

	AssetType	getType() const { return m_type; }
protected:
	MD5Hash 			m_uid = INVALID_ASSET_ID;
	AssetType			m_type;
};
