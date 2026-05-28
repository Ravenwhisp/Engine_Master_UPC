#include "Globals.h"
#include "ImporterPrefab.h"

#include "Prefab.h"
#include "PrefabInstanceComponent.h"
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

    JsonArchive goArchive(ArchiveMode::Input);
    goArchive.setValue(doc["GameObject"]);
    dst->GameObject::serialize(goArchive);

    dst->init();
    return true;
}
