#include "Globals.h"
#include "ImporterPrefab.h"

#include "Prefab.h"
#include "PrefabInstanceComponent.h"
#include "JsonArchive.h"


bool ImporterPrefab::saveNativeFile(const Prefab* asset, const std::filesystem::path& path)
{
    JsonArchive archive(ArchiveMode::Output);
    archive.setPrettyPrint(true);

    std::string pathStr = path.string();
    archive.serialize(pathStr, "SourcePath");
    std::string name = path.stem().string();
    archive.serialize(name, "Name");
    uint32_t version = 2;
    archive.serialize(version, "Version");

    auto* preComp = const_cast<Prefab*>(asset)->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
    if (preComp && preComp->isInstance() && preComp->getData().m_sourcePath != path)
    {
        std::string variantOf = preComp->getData().m_sourcePath.string();
        archive.serialize(variantOf, "VariantOf");
    }

    archive.beginObject("GameObject");
    const_cast<Prefab*>(asset)->serialize(archive);
    archive.endObject();

    const std::filesystem::path dir = path.parent_path();
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) return false;

    return archive.saveFile(path);
}

bool ImporterPrefab::importNative(const std::filesystem::path& path, Prefab* dst)
{
    JsonArchive archive(ArchiveMode::Input);
    if (!archive.loadFile(path))
    {
        DEBUG_ERROR("[ImporterPrefab] Failed to load '%s'.", path.string().c_str());
        return false;
    }

    if (!archive.hasKey("GameObject"))
    {
        DEBUG_ERROR("[ImporterPrefab] Invalid JSON in '%s' — missing GameObject.", path.string().c_str());
        return false;
    }

    archive.beginObject("GameObject");
    dst->serialize(archive);
    archive.endObject();
    dst->m_sourcePath = path;

    dst->init();
    return true;
}
