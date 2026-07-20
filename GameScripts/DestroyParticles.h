#pragma once

#include "ScriptAPI.h"

class DestroyParticles : public Script
{
    DECLARE_SCRIPT(DestroyParticles)

public:
    explicit DestroyParticles(GameObject* owner);

    void Start()  override;
    void Update() override;

    FieldList getExposedFields() const override;

public:
    float m_lifetime = 2.0f;

private:
    float m_timer = 0.0f;
};


