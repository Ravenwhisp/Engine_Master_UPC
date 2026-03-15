#include "Globals.h"
#include "ImporterPrefab.h"

#include "Application.h"
#include "ModuleFileSystem.h"

#include <fstream>
#include <sstream>

Asset* ImporterPrefab::createAssetInstance(const MD5Hash& uid) const
{
    return new PrefabAsset(uid);
}

bool ImporterPrefab::importNative(const std::filesystem::path& path, PrefabAsset* dst)
{
    const std::vector<uint8_t> raw = app->getModuleFileSystem()->read(path);
    if (raw.empty())
    {
        DEBUG_ERROR("[ImporterPrefab] Could not read '%s'.", path.string().c_str());
        return false;
    }

    dst->m_json.assign(reinterpret_cast<const char*>(raw.data()), raw.size());
    // For a native .prefab the asset UID doubles as the root UID.
    dst->m_rootUID = dst->m_uid;
    return true;
}

uint64_t ImporterPrefab::saveTyped(const PrefabAsset* src, uint8_t** outBuffer)
{
    const uint32_t jsonLen = static_cast<uint32_t>(src->m_json.size());
    const uint32_t rootLen = static_cast<uint32_t>(src->m_rootUID.size());
    const uint64_t totalSize = sizeof(uint32_t) + jsonLen + sizeof(uint32_t) + rootLen;

    uint8_t* buffer = new uint8_t[totalSize];
    BinaryWriter writer(buffer);
    writer.string(src->m_json);
    writer.string(src->m_rootUID);

    *outBuffer = buffer;
    return totalSize;
}

void ImporterPrefab::loadTyped(const uint8_t* buffer, PrefabAsset* dst)
{
    BinaryReader reader(buffer);
    dst->m_json = reader.string();
    dst->m_rootUID = reader.string();
}