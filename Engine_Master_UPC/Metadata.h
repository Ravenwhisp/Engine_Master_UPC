#pragma once
#include "MD5Fwd.h"
#include "UID.h"
#include "AssetType.h"
#include "AssetsDictionary.h"

#include <filesystem>
#include <vector>

struct DependencyRecord
{
	UID			uid = INVALID_UID;
	MD5Hash		contentHash = INVALID_ASSET_ID;
	AssetType	type = AssetType::UNKNOWN;
	std::string displayName;

	std::filesystem::path getBinaryPath() const
	{
		return buildLibraryPath(contentHash);
	}
};

struct Metadata
{
	UID			uid = INVALID_UID;
	MD5Hash		contentHash = INVALID_ASSET_ID;
	std::filesystem::path sourcePath;
	AssetType type = AssetType::UNKNOWN;

	uint64_t sourceFileSize = 0;

	std::vector<DependencyRecord> m_dependencies;
	bool m_isSubAsset = false;
	std::string displayName;


	std::filesystem::path getSourcePath(std::filesystem::path& metadataPath) const
	{
		return metadataPath.parent_path() / metadataPath.stem();
	}

	std::filesystem::path getBinaryPath() const
	{
		return buildLibraryPath(contentHash);
	}

	static void getMetadataPath(std::filesystem::path& assetPath)
	{
		assetPath += METADATA_EXTENSION;
	}
};