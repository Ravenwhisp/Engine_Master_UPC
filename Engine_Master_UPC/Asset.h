#pragma once
#include <string>
#include "filesystem"
#include <Logger.h>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <simdjson.h>
#include <fstream>

static const int INVALID_ASSET_ID = -1;

enum class AssetType : uint32_t
{
	TEXTURE = 0,
	MODEL = 1,
	MATERIAL = 2,
	MESH = 3,

	UNKNOWN = 4
};


class AssetMetadata
{
public:
	int uid = INVALID_ASSET_ID;
	AssetType type;
	std::filesystem::path sourcePath;
	std::filesystem::path binaryPath;

	// I AM SO SORRY FOR THIS
    static bool saveMetaFile(const AssetMetadata& meta, const std::filesystem::path& metaPath);
    static bool loadMetaFile(const std::filesystem::path& metaPath, AssetMetadata& outMeta);
};

class Asset
{
public:
	Asset() = default;
	Asset(int id, AssetType type = AssetType::UNKNOWN): m_uid(id), m_type(type) {}
	virtual ~Asset() = default;
	int getId() const { return m_uid; }

	void		addReference() { m_referenceCount += 1; }
	void		release() { m_referenceCount -= 1; }
	uint8_t		getReferenceCount() const { return m_referenceCount; }
	AssetType	getType() const { return m_type; }
protected:
	int			m_uid = INVALID_ASSET_ID;
	AssetType	m_type;
	uint8_t		m_referenceCount = 0;
};
