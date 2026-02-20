#include "Module.h"
#include "Importer.h"

class FileSystemModule : public Module
{
public:
	bool init() override;

	Asset * import(const char* filePath) const;

	unsigned int load(const char* filePath, char** buffer) const;
	unsigned int save(const char* filePath, const void* buffer, unsigned int size, bool append = false) const;
	bool copy(const char* sourceFilePath, const char* destinationFilePath) const;
	bool deleteFile(const char* filePath) const;
	bool createDirectory(const char* directoryPath) const;
	bool exists(const char* filePath) const;
	bool isDirectory(const char* path) const;
private:
	std::vector<Importer*> importers;
};