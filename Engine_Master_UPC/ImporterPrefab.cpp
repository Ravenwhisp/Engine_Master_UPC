#include "Globals.h"
#include "ImporterPrefab.h"

#include <fstream>
#include <sstream>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

Asset* ImporterPrefab::createAssetInstance(const MD5Hash& uid) const
{
    return new PrefabAsset(uid);
}


bool ImporterPrefab::importNative(const std::filesystem::path& path, PrefabAsset* dst)
{
    // Read the entire file into memory.
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        DEBUG_ERROR("[PrefabImporter] Cannot open '%s'.", path.string().c_str());
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string raw = ss.str();

    if (raw.empty())
    {
        DEBUG_ERROR("[PrefabImporter] Empty prefab file '%s'.", path.string().c_str());
        return false;
    }

    // Parse JSON to validate and extract RootUID.
    rapidjson::Document doc;
    doc.Parse(raw.c_str());

    if (doc.HasParseError())
    {
        DEBUG_ERROR("[PrefabImporter] JSON parse error in '%s' at offset %zu.",
            path.string().c_str(), doc.GetErrorOffset());
        return false;
    }

    if (!doc.HasMember("GameObjects") || !doc["GameObjects"].IsArray())
    {
        DEBUG_ERROR("[PrefabImporter] Missing 'GameObjects' array in '%s'.", path.string().c_str());
        return false;
    }

    // RootUID — the identifier of the top-level game object in the hierarchy.
    if (doc.HasMember("RootUID") && doc["RootUID"].IsString())
    {
        dst->m_rootUID = doc["RootUID"].GetString();
    }
    else
    {
        // Derive a stable root UID from the file path when the field is absent.
        dst->m_rootUID = computeMD5(path);
    }

    // Store the raw JSON string; instantiation code parses it on demand.
    dst->m_json = std::move(raw);

    return true;
}


uint64_t ImporterPrefab::saveTyped(const PrefabAsset* src, uint8_t** outBuffer)
{
    const uint32_t jsonLen = static_cast<uint32_t>(src->m_json.size());

    uint64_t size = 0;
    size += sizeof(uint32_t) + src->m_uid.size();     // uid
    size += sizeof(uint32_t) + src->m_rootUID.size(); // rootUID
    size += sizeof(uint32_t);                         // jsonByteLength
    size += jsonLen;                                  // json payload

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter w(buffer);

    w.string(src->m_uid);
    w.string(src->m_rootUID);
    w.u32(jsonLen);
    w.bytes(src->m_json.data(), jsonLen);

    *outBuffer = buffer;
    return size;
}


void ImporterPrefab::loadTyped(const uint8_t* buffer, PrefabAsset* dst)
{
    BinaryReader r(buffer);

    dst->m_uid = r.string();
    dst->m_rootUID = r.string();

    const uint32_t jsonLen = r.u32();
    dst->m_json.assign(reinterpret_cast<const char*>(r.ptr()), jsonLen);
}