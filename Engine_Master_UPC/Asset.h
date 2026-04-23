#pragma once
#include "ISerializable.h"
#include "MD5Fwd.h"
#include "AssetsDictionary.h"
#include "AssetType.h"

class Asset: public ISerializable
{
public:
	Asset() = default;
	Asset(MD5Hash id, AssetType type = AssetType::UNKNOWN): m_uid(id), m_type(type) {}
	virtual ~Asset() = default;
	MD5Hash getId() const { return m_uid; }

#pragma region Persistence
	bool toJson(rapidjson::Document& domTree) const override { return false; }
	bool fromJson(const rapidjson::Value& json) override { return false; }
	AssetType	getAssetType() const override { return m_type; }
#pragma endregion
protected:
	MD5Hash 			m_uid = INVALID_ASSET_ID;
	AssetType			m_type = AssetType::UNKNOWN;
};
