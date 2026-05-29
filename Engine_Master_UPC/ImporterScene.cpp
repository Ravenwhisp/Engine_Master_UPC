#include <Globals.h>
#include "ImporterScene.h"
#include "JsonArchive.h"


Asset* ImporterScene::createAssetInstance(AssetReference& uid) const
{
	return new Scene(uid);
}

bool ImporterScene::saveNative(const Scene* asset, const std::filesystem::path& path)
{
    JsonArchive archive(ArchiveMode::Output);
    const_cast<Scene*>(asset)->serialize(archive);
    return archive.saveFile(path);
}

bool ImporterScene::importNative(const std::filesystem::path& path, Scene* dst)
{
    JsonArchive archive(ArchiveMode::Input);
    if (!archive.loadFile(path))
    {
        DEBUG_ERROR("[ImporterScene] Failed to load file: %s", path.string().c_str());
        return false;
    }
    dst->serialize(archive);
    return true;
}
