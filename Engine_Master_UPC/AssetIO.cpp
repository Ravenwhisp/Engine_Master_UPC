#include "Globals.h"
#include "AssetIO.h"

#include "MD5.h"
#include "AssetType.h"
#include "ImportSettings.h"
#include "Metadata.h"
#include "JsonArchive.h"

#include <filesystem>

namespace AssetIO
{

bool saveMetaFile(const Metadata& meta, const std::filesystem::path& metaPath)
{
    JsonArchive archive;
    const_cast<Metadata&>(meta).serialize(archive);
    return archive.saveFile(metaPath);
}

bool loadMetaFile(const std::filesystem::path& metaPath, Metadata& outMeta)
{
    JsonArchive archive(ArchiveMode::Input);
    if (!archive.loadFile(metaPath))
    {
        DEBUG_ERROR("[AssetIO] Could not load '%s'.", metaPath.string().c_str());
        return false;
    }
    outMeta.serialize(archive);
    if (!isValidUID(outMeta.uid))
    {
        DEBUG_ERROR("[AssetIO] Missing 'uid' in '%s'.", metaPath.string().c_str());
        return false;
    }
    return true;
}

} // namespace AssetIO
