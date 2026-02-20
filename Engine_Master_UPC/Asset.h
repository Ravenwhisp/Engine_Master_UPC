#pragma once
#include <string>

class Asset
{
public:
	Asset(int id);
	virtual ~Asset() = default;
	int getId() const { return id; }
private:
	int id;
	std::string assetsFile;
	std::string libraryFile;

	uint8_t referenceCount = 0;
};