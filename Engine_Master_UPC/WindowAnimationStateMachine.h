#pragma once
#include "EditorWindow.h"
#include "MD5Fwd.h"

#include <memory>

class AnimationStateMachineAsset;

class WindowAnimationStateMachine final : public EditorWindow
{
public:
    WindowAnimationStateMachine();
    ~WindowAnimationStateMachine() override = default;

    const char* getWindowName() const override
    {
        return "Animation State Machine";
    }

    void setTargetStateMachineUID(const MD5Hash& uid);
    const MD5Hash& getTargetStateMachineUID() const
    {
        return m_targetStateMachineUID;
    }

protected:
    void drawInternal() override;

private:
    bool ensureAssetLoaded();

private:
    MD5Hash m_targetStateMachineUID = INVALID_ASSET_ID;
    std::shared_ptr<AnimationStateMachineAsset> m_asset;
};
