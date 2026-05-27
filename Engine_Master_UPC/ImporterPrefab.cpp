#include "Globals.h"
#include "ImporterPrefab.h"

#include "Application.h"


#include <rapidjson/document.h>
#include <FileIO.h>

Asset* ImporterPrefab::createAssetInstance(AssetReference& uid) const
{
    return new PrefabAsset(uid);
}

bool ImporterPrefab::saveNative(const PrefabAsset* asset, const std::filesystem::path& path)
{
    return false;
}

bool ImporterPrefab::importNative(const std::filesystem::path& path, PrefabAsset* dst)
{
    const std::vector<uint8_t> raw = FileIO::read(path);
    if (raw.empty())
    {
        DEBUG_ERROR("[ImporterPrefab] Could not read '%s'.", path.string().c_str());
        return false;
    }

    PrefabData& data = dst->getData();
    data.m_json.assign(reinterpret_cast<const char*>(raw.data()), raw.size());
    data.m_assetUID = dst->m_reference.m_uid;
    data.m_sourcePath = path;
    data.m_name = path.stem().string();

    // Extract PrefabUID (uint64_t GO UID) from the top-level JSON key.
    rapidjson::Document doc;
    doc.Parse(data.m_json.c_str());
    if (!doc.HasParseError())
    {
        // Allow the JSON to override the display name.
        if (doc.HasMember("Name") && doc["Name"].IsString())
        {
            data.m_name = doc["Name"].GetString();
        }

    }

    return true;
}