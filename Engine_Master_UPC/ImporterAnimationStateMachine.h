#pragma once
#include "ImporterNative.h"
#include "AnimationStateMachineAsset.h"

class ImporterAnimationStateMachine
    : public ImporterNative<AnimationStateMachineAsset, AssetType::ANIMATION_STATE_MACHINE>
{
public:
    bool canImport(const std::filesystem::path&) const override
    {
        return false;
    }

    Asset* createAssetInstance(const MD5Hash& uid) const override
    {
        return new AnimationStateMachineAsset(uid);
    }

protected:
    bool importNative(const std::filesystem::path&, AnimationStateMachineAsset*) override
    {
        return false;
    }

    uint64_t saveTyped(const AnimationStateMachineAsset* source, uint8_t** outBuffer) override;
    void loadTyped(const uint8_t* buffer, AnimationStateMachineAsset* dst) override;
};
