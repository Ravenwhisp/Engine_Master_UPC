#include <Globals.h>
#include "ImporterScene.h"
#include <SceneSerializer.h>

#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>


Asset* ImporterScene::createAssetInstance(AssetReference& uid) const
{
	return new Scene(uid);
}

bool ImporterScene::saveNative(const Scene* asset, const std::filesystem::path& path)
{
    rapidjson::Document domTree;
    domTree.SetObject();

    rapidjson::Value sceneValue = SceneSerializer::getJSON(domTree, asset);

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

bool ImporterScene::importNative(const std::filesystem::path& path, Scene* dst)
{
    const std::string pathStr = path.string();
    FILE* fp = std::fopen(pathStr.c_str(), "rb");
    if (!fp)
    {
        DEBUG_ERROR("[AssetMetadata] Could not open '%s'.", pathStr.c_str());
        return false;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document doc;
    doc.ParseStream(is);
    std::fclose(fp);

    if (doc.HasParseError())
    {
        DEBUG_ERROR("[SceneSerializer] JSON parse error");
        return false;
    }

    if (!doc.IsObject())
    {
        DEBUG_ERROR("[SceneSerializer] Scene JSON root is not an object");
        return false;
    }

    if (!SceneSerializer::LoadFromJSON(*dst, doc))
    {
        DEBUG_ERROR("[SceneSerializer] Failed to load scene from JSON");
        return false;
    }

	return true;
}

uint64_t ImporterScene::saveTyped(const Scene* source, uint8_t** outBuffer)
{
	return 0;
}

void ImporterScene::loadTyped(const uint8_t* buffer, Scene* dst)
{

}
