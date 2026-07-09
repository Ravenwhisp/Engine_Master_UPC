#pragma once

#include "ScriptAPI.h"

class Transform;

class AnimationOverrideTester : public Script
{
    DECLARE_SCRIPT(AnimationOverrideTester)

public:
    explicit AnimationOverrideTester(GameObject* owner);

    void Update() override;

    FieldList getExposedFields() const override;

private:
    AnimationComponent* getTargetAnimation() const;

public:
    ComponentRef<Transform> m_target;

    std::string m_overrideClipName = "Walk";

    bool m_loopOverrideClip = true;
};