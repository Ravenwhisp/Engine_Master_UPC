#pragma once
#include <string>
#include "filesystem"
#include <Logger.h>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include "rapidjson/filereadstream.h"

#include <simdjson.h>
#include <fstream>
#include <UID.h>

#include <AssetsDictionary.h>


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
	UID uid = INVALID_ASSET_ID;
	AssetType type;

	std::filesystem::path getSourcePath(std::filesystem::path& metadataPath) const
	{
		return metadataPath.parent_path() / metadataPath.stem();
	}

	std::filesystem::path getBinaryPath() const
	{
		return std::filesystem::path(LIBRARY_FOLDER) / std::to_string(uid) += ".asset";
	}

	static void getMetadataPath(std::filesystem::path& assetPath) 
	{
		assetPath += METADATA_EXTENSION;
	}

	// I AM SO SORRY FOR THIS
    static bool saveMetaFile(const AssetMetadata& meta, const std::filesystem::path& metaPath);
    static bool loadMetaFile(const std::filesystem::path& metaPath, AssetMetadata& outMeta);
};

class Asset
{
public:
	Asset() = default;
	Asset(UID id, AssetType type = AssetType::UNKNOWN): m_uid(id), m_type(type) {}
	virtual ~Asset() = default;
	UID getId() const { return m_uid; }

	void		addReference() { m_referenceCount += 1; }
	void		release() { m_referenceCount -= 1; }
	uint8_t		getReferenceCount() const { return m_referenceCount; }
	AssetType	getType() const { return m_type; }
protected:
	UID			m_uid = INVALID_ASSET_ID;
	AssetType	m_type;
	uint8_t		m_referenceCount = 0;
};
