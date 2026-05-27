#pragma once
#include "MD5Fwd.h"
#include "UID.h"
#include "AssetType.h"
#include "AssetsDictionary.h"
#include "ImportSettings.h"

#include <filesystem>
#include <vector>
#include <memory>

struct DependencyRecord
{
	UID			uid = INVALID_UID;
	MD5Hash		contentHash = INVALID_ASSET_ID;
	AssetType	type = AssetType::UNKNOWN;
	std::string displayName;

	std::filesystem::path getBinaryPath() const
	{
		return std::filesystem::path(LIBRARY_FOLDER) / contentHash += ASSET_EXTENSION;
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

	std::unique_ptr<ImportSettings> importSettings;

	Metadata() = default;
	Metadata(const Metadata& other)
		: uid(other.uid)
		, contentHash(other.contentHash)
		, sourcePath(other.sourcePath)
		, type(other.type)
		, sourceFileSize(other.sourceFileSize)
		, m_dependencies(other.m_dependencies)
		, m_isSubAsset(other.m_isSubAsset)
		, displayName(other.displayName)
		, importSettings(other.importSettings ? other.importSettings->clone() : nullptr)
	{
	}
	Metadata& operator=(const Metadata& other)
	{
		if (this != &other)
		{
			uid = other.uid;
			contentHash = other.contentHash;
			sourcePath = other.sourcePath;
			type = other.type;
			sourceFileSize = other.sourceFileSize;
			m_dependencies = other.m_dependencies;
			m_isSubAsset = other.m_isSubAsset;
			displayName = other.displayName;
			importSettings = other.importSettings ? other.importSettings->clone() : nullptr;
		}
		return *this;
	}

	Metadata(Metadata&&) = default;
	Metadata& operator=(Metadata&&) = default;

	std::filesystem::path getSourcePath(std::filesystem::path& metadataPath) const
	{
		return metadataPath.parent_path() / metadataPath.stem();
	}

	std::filesystem::path getBinaryPath() const
	{
		return std::filesystem::path(LIBRARY_FOLDER) / contentHash += ASSET_EXTENSION;
	}

	static void getMetadataPath(std::filesystem::path& assetPath)
	{
		assetPath += METADATA_EXTENSION;
	}

	bool load(const std::filesystem::path& metaPath);
	bool save(const std::filesystem::path& metaPath) const;
};
