#pragma once
#include "MD5Fwd.h"
#include "AssetType.h"
#include "AssetsDictionary.h"

struct DependencyRecord
{
	MD5Hash   uid = INVALID_ASSET_ID;
	AssetType type = AssetType::UNKNOWN;
};

struct Metadata
{
	MD5Hash uid = INVALID_ASSET_ID;
	std::filesystem::path sourcePath;
	AssetType type;

	std::vector<DependencyRecord> m_dependencies;
	bool m_isSubAsset = false;


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
};