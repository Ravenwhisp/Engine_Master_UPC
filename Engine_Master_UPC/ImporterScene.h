#pragma once
#include "ImporterNative.h"

#include "Scene.h"

class ImporterScene : public ImporterNative<Scene, AssetType::SCENE>
{
public:
    bool canImport(const std::filesystem::path& path) const override
    {
        return path.extension().string() == SCENE_EXTENSION;
    }

    Asset* createAssetInstance(AssetReference& uid) const override;
    bool saveNative(const Scene* asset, const std::filesystem::path& path);
protected:
    bool     importNative(const std::filesystem::path& path, Scene* dst) override;
    uint64_t saveTyped(const Scene* source, uint8_t** outBuffer)      override;
    void     loadTyped(const uint8_t* buffer, Scene* dst)         override;
};