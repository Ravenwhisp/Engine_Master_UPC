#include "Globals.h"
#include "ImporterPrefab.h"

#include "Prefab.h"
#include "PrefabManager.h"
#include "PrefabInstanceComponent.h"
#include "Transform.h"
#include "JsonArchive.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <FileIO.h>

using namespace rapidjson;

Asset* ImporterPrefab::createAssetInstance(AssetReference& uid) const
{
    return new Prefab(uid);
}

bool ImporterPrefab::saveNative(const Prefab* asset, const std::filesystem::path& path)
{
    Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("SourcePath", Value(path.string().c_str(), alloc), alloc);
    doc.AddMember("Name", Value(path.stem().string().c_str(), alloc), alloc);
    doc.AddMember("Version", 2, alloc);

    auto* preComp = const_cast<Prefab*>(asset)->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
    if (preComp && preComp->isInstance() && preComp->getData().m_sourcePath != path)
        doc.AddMember("VariantOf", Value(preComp->getData().m_sourcePath.string().c_str(), alloc), alloc);

    JsonArchive goArchive;
    const_cast<Prefab*>(asset)->serialize(goArchive);
    Value goNode = goArchive.extractValue(doc.GetAllocator());
    doc.AddMember("GameObject", goNode, alloc);

    const std::filesystem::path dir = path.parent_path();
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) return false;

    FILE* file = fopen(path.string().c_str(), "wb");
    if (!file) return false;

    StringBuffer sb;
    Writer<StringBuffer> writer(sb);
    doc.Accept(writer);
    fwrite(sb.GetString(), 1, sb.GetSize(), file);
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

    // Inline deserialiseTransform
    if (goNode.HasMember("Transform") && goNode["Transform"].IsObject())
    {
        JsonArchive tfArchive(ArchiveMode::Input);
        tfArchive.setValue(goNode["Transform"]);
        dst->GetTransform()->serialize(tfArchive);
    }

    // Inline deserialiseComponents
    if (goNode.HasMember("Components") && goNode["Components"].IsArray())
    {
        for (SizeType i = 0; i < goNode["Components"].Size(); ++i)
        {
            const auto& cn = goNode["Components"][i];
            auto type = static_cast<ComponentType>(cn["Type"].GetInt());
            Component* comp = dst->AddComponentWithUID(type, GenerateUID());
            if (comp && cn.HasMember("Data") && cn["Data"].IsObject())
            {
                JsonArchive compArchive(ArchiveMode::Input);
                compArchive.setValue(cn["Data"]);
                comp->serialize(compArchive);
            }
        }
    }

    // Children via tree-building helper
    if (goNode.HasMember("Children") && goNode["Children"].IsArray())
    {
        for (SizeType i = 0; i < goNode["Children"].Size(); ++i)
            PrefabManager::createFromJSON(goNode["Children"][i], dst);
    }

    dst->init();
    return true;
}
