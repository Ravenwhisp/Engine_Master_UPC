#pragma once
#include "ImporterNative.h"
#include "AnimationStateMachineAsset.h"

class ImporterAnimationStateMachine
    : public ImporterNative<AnimationStateMachineAsset, AssetType::ANIMATION_STATE_MACHINE>
{
public:
    bool canImport(const std::filesystem::path& path) const override
    {
        return path.extension() == ".statemachine";
    }

    Asset* createAssetInstance(AssetReference& uid) const override
    {
        return new AnimationStateMachineAsset(uid);
    }
    bool saveNative(const AnimationStateMachineAsset* asset, const std::filesystem::path& path);

protected:
    bool importNative(const std::filesystem::path& path, AnimationStateMachineAsset* dst) override;

    uint64_t saveTyped(const AnimationStateMachineAsset* source, uint8_t** outBuffer) override;
    void loadTyped(const uint8_t* buffer, AnimationStateMachineAsset* dst) override;
};