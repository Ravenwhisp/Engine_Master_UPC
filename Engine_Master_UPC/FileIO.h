#pragma once
#include <filesystem>

class FileIO {
public: 
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
};
