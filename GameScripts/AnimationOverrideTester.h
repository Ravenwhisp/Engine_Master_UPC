#pragma once

#include "ScriptAPI.h"

class Transform;

class AnimationOverrideTester : public Script
{
    DECLARE_SCRIPT(AnimationOverrideTester)

public:
    explicit AnimationOverrideTester(GameObject* owner);

    void Update() override;

    ScriptFieldList getExposedFields() const override;

private:
    AnimationComponent* getTargetAnimation() const;

public:
    ScriptComponentRef<Transform> m_target;

    std::string m_overrideClipName = "Walk";

    bool m_loopOverrideClip = true;
};