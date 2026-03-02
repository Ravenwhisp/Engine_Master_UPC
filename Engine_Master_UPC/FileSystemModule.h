#pragma once
#include "Module.h"
#include "Importer.h"

#include "Asset.h"

struct FileEntry {
	std::filesystem::path path;
	std::string displayName;
	UID uid = INVALID_ASSET_ID;
	bool isDirectory;
	std::vector<std::shared_ptr<FileEntry>> children;

	std::filesystem::path getPath() 
	{
		if (isDirectory)
		{
			std::filesystem::path realPath = path;
			realPath += "/";
			return realPath;
		}
		return path;
	}
};

struct PendingImport
{
	std::filesystem::path sourcePath;
	UID existingUID = INVALID_ASSET_ID;
};

class FileSystemModule : public Module
{
public:
	bool init() override;

	Importer* findImporter(const std::filesystem::path& filePath);
	Importer* findImporter(const char* filePath);

	Importer* findImporter(AssetType type);
	AssetMetadata* getMetadata(UID uid);

	unsigned int load(const std::filesystem::path& filePath, char** buffer) const;
	unsigned int load(const char* filePath, char** buffer) const;

	unsigned int save(const std::filesystem::path& filePath, const void* buffer, unsigned int size, bool append = false) const;
	unsigned int save(const char* filePath, const void* buffer, unsigned int size, bool append = false) const;

	bool copy(const char* sourceFilePath, const char* destinationFilePath) const;
	bool move(const char* sourceFilePath, const char* destinationFilePath) const;
	bool deleteFile(const char* filePath) const;
	bool createDirectory(const char* directoryPath) const;
	bool exists(const char* filePath) const;
	bool isDirectory(const char* path) const;

#pragma region FileDialog
	void rebuild();
	std::shared_ptr<FileEntry> getRoot() { return m_root; }
	std::shared_ptr<FileEntry> getEntry(const std::filesystem::path& path);
private:
	std::shared_ptr<FileEntry> getEntryRecursive(const std::shared_ptr<FileEntry>& node, const std::filesystem::path& path) const;
	std::shared_ptr<FileEntry> buildTree(const std::filesystem::path& path);
	std::shared_ptr<FileEntry> buildDirectoryEntry(const std::filesystem::path& path);
	std::shared_ptr<FileEntry> buildMetadataEntry(const std::filesystem::path& path);

	void checkFile(const std::filesystem::path& path);
	void loadMetadata(const std::filesystem::path& path);

	void handleMissingMetadata(const std::filesystem::path& path);
	void handleOrphanedMetadata(const std::filesystem::path& metadataPath);
	std::filesystem::path getBinaryPath(UID uid) const;
	void cleanOrphanedBinaries();
	std::shared_ptr<FileEntry> m_root;
#pragma endregion
	std::unordered_map<UID, AssetMetadata> m_metadataMap;
	std::vector<PendingImport> m_pendingImports;

	// I don't know if having to ways of finding an importer is the solution
	std::unordered_map<AssetType, Importer*> importersMap;
	std::vector<Importer*> importers;
};