#include "Globals.h"
#include "ImporterPrefab.h"

#include "Application.h"
#include "ModuleFileSystem.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"

#include <rapidjson/document.h>

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

    PrefabData& data = dst->getData();
    data.m_json.assign(reinterpret_cast<const char*>(raw.data()), raw.size());
    data.m_assetUID = dst->m_uid;
    data.m_sourcePath = path;
    data.m_name = path.stem().string();

    // Extract PrefabUID (uint64_t GO UID) from the top-level JSON key.
    rapidjson::Document doc;
    doc.Parse(data.m_json.c_str());
    if (!doc.HasParseError())
    {
        if (doc.HasMember("PrefabUID") && doc["PrefabUID"].IsUint64())
        {
            data.m_prefabUID = static_cast<UID>(doc["PrefabUID"].GetUint64());
        }

        // Allow the JSON to override the display name.
        if (doc.HasMember("Name") && doc["Name"].IsString())
        {
            data.m_name = doc["Name"].GetString();
        }

    }

    return true;
}

uint64_t ImporterPrefab::saveTyped(const PrefabAsset* src, uint8_t** outBuffer)
{
    const PrefabData& data = src->getData();
    const std::string  pathStr = data.m_sourcePath.string();

    uint64_t size = 0;
    size += sizeof(uint32_t) + pathStr.size();
    size += sizeof(uint32_t) + data.m_name.size();
    size += sizeof(uint32_t) + data.m_assetUID.size();
    size += sizeof(uint64_t);
    size += sizeof(uint32_t) + data.m_json.size();

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter writer(buffer);
    writer.string(pathStr);
    writer.string(data.m_name);
    writer.string(data.m_assetUID);
    writer.u64(static_cast<uint64_t>(data.m_prefabUID));
    writer.string(data.m_json);

    *outBuffer = buffer;
    return size;
}

void ImporterPrefab::loadTyped(const uint8_t* buffer, PrefabAsset* dst)
{
    PrefabData& data = dst->getData();
    BinaryReader reader(buffer);

    data.m_sourcePath = reader.string();
    data.m_name = reader.string();
    data.m_assetUID = reader.string();
    data.m_prefabUID = static_cast<UID>(reader.u64());
    data.m_json = reader.string();
}