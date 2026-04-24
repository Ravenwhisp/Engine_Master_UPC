#include "Globals.h"
#include "Scene.h"
#include "ImporterScene.h"
#include <SceneSerializer.h>

#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>

Asset* ImporterScene::createAssetInstance(const MD5Hash& uid) const
{
	return new Scene(uid);
}

bool ImporterScene::importNative(const std::filesystem::path& path, Scene* dst)
{
	return false;
}

bool ImporterScene::saveNative(const std::filesystem::path& path, const Scene* src)
{

    rapidjson::Document domTree;
    domTree.SetObject();

    rapidjson::Value sceneValue = SceneSerializer::getJSON(domTree, src);

    domTree.Swap(sceneValue);

    FILE* fileOpened = std::fopen(path.string().c_str(), "wb");
    if (!fileOpened)
    {
        DEBUG_ERROR("[SceneSerializer] Failed to open file for writing: %s", path.c_str());
        return false;
    }

    char writeBuffer[65536];
    rapidjson::FileWriteStream stream(fileOpened, writeBuffer, sizeof(writeBuffer));

    rapidjson::Writer<rapidjson::FileWriteStream> writer(stream);
    domTree.Accept(writer);

    std::fclose(fileOpened);

	return true;
}

uint64_t ImporterScene::saveTyped(const Scene* source, uint8_t** outBuffer)
{
	return 0;
}

void ImporterScene::loadTyped(const uint8_t* buffer, Scene* dst)
{
}
