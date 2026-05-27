#include "Globals.h"
#include "ImporterPrefab.h"

#include "Application.h"
#include "PrefabSerializer.h"
#include "Prefab.h"

#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/writer.h>
#include <FileIO.h>

using namespace rapidjson;

Asset* ImporterPrefab::createAssetInstance(AssetReference& uid) const
{
    return new Prefab(uid);
}

bool ImporterPrefab::saveNative(const Prefab* asset, const std::filesystem::path& path)
{
    // Serialize the Prefab's GameObject tree to a .prefab JSON file.
    const std::string json = PrefabSerializer::buildPrefabJSON(asset, path);
    if (json.empty()) return false;

    const std::filesystem::path dir = path.parent_path();
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) return false;

    FILE* file = fopen(path.string().c_str(), "wb");
    if (!file) return false;

    fwrite(json.data(), 1, json.size(), file);
    fclose(file);
    return true;
}

bool ImporterPrefab::importNative(const std::filesystem::path& path, Prefab* dst)
{
    const std::vector<uint8_t> raw = FileIO::read(path);
    if (raw.empty())
    {
        DEBUG_ERROR("[ImporterPrefab] Could not read '%s'.", path.string().c_str());
        return false;
    }

    rapidjson::Document doc;
    doc.Parse(reinterpret_cast<const char*>(raw.data()));
    if (doc.HasParseError() || !doc.HasMember("GameObject"))
    {
        DEBUG_ERROR("[ImporterPrefab] Invalid JSON in '%s'.", path.string().c_str());
        return false;
    }

    dst->m_sourcePath = path;

    const auto& goNode = doc["GameObject"];

    dst->SetName(goNode.HasMember("Name") ? goNode["Name"].GetString() : "Unnamed");
    dst->SetActive(goNode.HasMember("Active") ? goNode["Active"].GetBool() : true);
    if (goNode.HasMember("Tag") && goNode["Tag"].IsString())
        dst->SetTag(StringToTag(goNode["Tag"].GetString()));
    if (goNode.HasMember("Layer") && goNode["Layer"].IsString())
        dst->SetLayer(StringToLayer(goNode["Layer"].GetString()));

    PrefabSerializer::deserialiseTransform(goNode, dst);
    PrefabSerializer::deserialiseComponents(goNode, dst);

    if (goNode.HasMember("Children") && goNode["Children"].IsArray())
    {
        for (SizeType i = 0; i < goNode["Children"].Size(); ++i)
            PrefabSerializer::deserialiseNode(goNode["Children"][i], dst);
    }

    dst->init();
    return true;
}
