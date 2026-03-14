#pragma once
#include <string>
#include "filesystem"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include "rapidjson/filereadstream.h"

#include <fstream>
#include "MD5.h" 
#include "AssetsDictionary.h"

enum class AssetType : uint32_t
{
	TEXTURE = 0,
	MODEL = 1,
	MATERIAL = 2,
	MESH = 3,
	FONT = 4,

	UNKNOWN = 4
};


class AssetMetadata
{
public:
	MD5Hash uid = INVALID_ASSET_ID;
	std::filesystem::path sourcePath;
	AssetType type;

	std::filesystem::path getSourcePath(std::filesystem::path& metadataPath) const
	{
		return metadataPath.parent_path() / metadataPath.stem();
	}

	std::filesystem::path getBinaryPath() const
	{
		return std::filesystem::path(LIBRARY_FOLDER) / uid += ASSET_EXTENSION;
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
	Asset(MD5Hash id, AssetType type = AssetType::UNKNOWN): m_uid(id), m_type(type) {}
	virtual ~Asset() = default;
	MD5Hash getId() const { return m_uid; }

	AssetType	getType() const { return m_type; }
protected:
	MD5Hash 			m_uid = INVALID_ASSET_ID;
	AssetType	m_type;
};
