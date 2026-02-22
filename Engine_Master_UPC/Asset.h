#pragma once
#include <string>

class Asset
{
public:
	Asset() = default;
	Asset(int id): id(id) {}
	virtual ~Asset() = default;
	int getId() const { return id; }

	std::string getAssetsFile() const { return assetsFile; }
	std::string getLibraryFile() const { return libraryFile; }

protected:
	int id;
	std::string assetsFile;
	std::string libraryFile;

	uint8_t referenceCount = 0;
};