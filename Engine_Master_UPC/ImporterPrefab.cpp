#include "Globals.h"
#include "ImporterPrefab.h"

#include "Application.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"

#include <rapidjson/document.h>
#include <FileIO.h>

#include "ComponentsCommon.h"

Asset* ImporterPrefab::createAssetInstance(const UID& uid) const
{
    return new PrefabAsset(uid);
}

bool ImporterPrefab::importNative(const std::filesystem::path& path, PrefabAsset* dst)
{
    const std::vector<uint8_t> raw = FileIO::read(path);
    if (raw.empty())
    {
        DEBUG_ERROR("[ImporterPrefab] Could not read '%s'.", path.string().c_str());
        return false;
    }

    return true;
}

bool ImporterPrefab::saveNative(const std::filesystem::path& path, const PrefabAsset* src)
{
    return false;
}

uint64_t ImporterPrefab::saveTyped(const PrefabAsset* src, uint8_t** outBuffer)
{
    return CerealUtils::saveTo(*src, outBuffer);
}

void ImporterPrefab::loadTyped(const uint8_t* buffer, PrefabAsset* dst)
{
    CerealUtils::loadFrom(buffer, *dst);
}