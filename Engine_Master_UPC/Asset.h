#pragma once
#include <string>

class Asset
{
public:
	Asset() = default;
	Asset(int id): id(id) {}
	virtual ~Asset() = default;
	int getId() const { return id; }
protected:
	int id;
	std::string assetsFile;
	std::string libraryFile;

	uint8_t referenceCount = 0;
};