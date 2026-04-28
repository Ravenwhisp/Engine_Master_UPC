#pragma once
#include "ImporterNative.h"
#include "AnimationStateMachineAsset.h"

class ImporterAnimationStateMachine: public ImporterNative<AnimationStateMachineAsset, AssetType::ANIMATION_STATE_MACHINE>
{
public:
    bool canImport(const std::filesystem::path& path) const override
    {
        return path.extension() == ".statemachine";
    }

    Asset* createAssetInstance(const UID& uid) const override
    {
        return new AnimationStateMachineAsset(uid);
    }

protected:
    bool importNative(const std::filesystem::path& path, AnimationStateMachineAsset* dst) override;
    bool saveNative(const std::filesystem::path& path, const AnimationStateMachineAsset* src) override;
    uint64_t saveTyped(const AnimationStateMachineAsset* source, uint8_t** outBuffer) override;
    void loadTyped(const uint8_t* buffer, AnimationStateMachineAsset* dst) override;
};