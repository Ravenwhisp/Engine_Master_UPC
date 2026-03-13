#pragma once
#include "Module.h"
#include "Asset.h"
#include "AssetScanner.h"
#include "ContentRegistry.h"
#include "MetadataStore.h"
#include "ImporterRegistry.h"
#include "Importer.h"
#include "FileIO.h"

class AssetMetadata;
struct FileEntry;


class ModuleFileSystem : public Module
{
public:
	bool init() override;

#pragma region FileIO
	unsigned int load(const std::filesystem::path& filePath, char** buffer) const;
	unsigned int load(const char* filePath, char** buffer) const;

	unsigned int save(const std::filesystem::path& filePath, const void* buffer, unsigned int size, bool append = false) const;
	unsigned int save(const char* filePath, const void* buffer, unsigned int size, bool append = false) const;

	bool copy(const char* sourceFilePath, const char* destinationFilePath) const;
	bool move(const char* sourceFilePath, const char* destinationFilePath) const;
	bool deleteFile(const char* filePath)        const;
	bool createDirectory(const char* path)       const;
	bool exists(const char* filePath)            const;
	bool isDirectory(const char* path)           const;
#pragma endregion

#pragma region Importer
	Importer* findImporter(const std::filesystem::path& filePath) const;
	Importer* findImporter(const char* filePath)                   const;
	Importer* findImporter(AssetType type)                         const;
#pragma endregion

#pragma region Metadata
	AssetMetadata* getMetadata(UID uid);
	UID            findByPath(const std::filesystem::path& sourcePath) const;
	void           registerMetadata(const AssetMetadata& meta, const std::filesystem::path& sourcePath);
#pragma endregion

#pragma region FileDialog
	void rebuild();
	std::shared_ptr<FileEntry> getRoot()                                const;
	std::shared_ptr<FileEntry> getEntry(const std::filesystem::path&)   const;

	OnImportRequestedEvent& onImportRequested() { return m_scanner->OnImportRequested; }

	DelegateHandle subscribeToImportRequested(OnImportRequestedEvent::DelegateT&& delegate)
	{
		return m_scanner->OnImportRequested.Add(std::move(delegate));
	}

	void unsubscribeFromImportRequested(DelegateHandle& handle)
	{
		m_scanner->unsubscribe(handle);
	}

#pragma endregion
private:
	std::unique_ptr<FileIO> 		 m_fileIO;
	std::unique_ptr<MetadataStore>   m_metadataStore;
	std::unique_ptr<ImporterRegistry> m_importerRegistry;
	std::unique_ptr<AssetScanner>    m_scanner;
	std::unique_ptr<ContentRegistry> m_contentRegistry;
};