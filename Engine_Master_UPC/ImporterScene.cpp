#include "Globals.h"
#include "Scene.h"
#include "ImporterScene.h"

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
	return false;
}

uint64_t ImporterScene::saveTyped(const Scene* source, uint8_t** outBuffer)
{
	return 0;
}

void ImporterScene::loadTyped(const uint8_t* buffer, Scene* dst)
{
}
