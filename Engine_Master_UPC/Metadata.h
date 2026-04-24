#pragma once

#include "MD5Fwd.h"
#include "AssetType.h"
#include "AssetsDictionary.h"
#include <filesystem>

struct DependencyRecord
{
	MD5Hash   uid = INVALID_ASSET_ID;
	AssetType type = AssetType::UNKNOWN;
};

class Metadata
{
public:
	MD5Hash uid = INVALID_ASSET_ID;
	std::filesystem::path sourcePath;
	AssetType type = AssetType::METADATA;

	std::vector<DependencyRecord> m_dependencies;
	bool m_isSubAsset = false;

#pragma region Persistence
	bool toJson(rapidjson::Document& doc) const ;
	bool fromJson(const rapidjson::Value& json);
#pragma endregion

	static std::filesystem::path getSourcePath(std::filesystem::path& metadataPath)
	{
		return metadataPath.parent_path() / metadataPath.stem();
	}

	std::filesystem::path getBinaryPath() const
	{
		return std::filesystem::path(LIBRARY_FOLDER) / uid += ASSET_EXTENSION;
	}

	static std::filesystem::path toMetadataPath(std::filesystem::path assetPath)
	{
		return assetPath += METADATA_EXTENSION;
	}


};