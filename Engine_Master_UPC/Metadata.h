#pragma once

#include "MD5Fwd.h"
#include "UID.h"
#include "AssetType.h"
#include "AssetsDictionary.h"
#include <filesystem>


struct DependencyRecord
{
	UID localId = INVALID_UID;
	AssetType type = AssetType::UNKNOWN;
};

class Metadata
{
public:
	UID fileId = INVALID_UID;
	MD5Hash contentHash = INVALID_ASSET_ID;

	std::filesystem::path sourcePath;
	AssetType type = AssetType::METADATA;

	std::vector<DependencyRecord> m_dependencies;
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
		return std::filesystem::path(LIBRARY_FOLDER) / contentHash += ASSET_EXTENSION;
	}

	static std::filesystem::path toMetadataPath(std::filesystem::path assetPath)
	{
		return assetPath += METADATA_EXTENSION;
	}
};