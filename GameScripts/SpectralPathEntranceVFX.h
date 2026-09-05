#pragma once

#include "ScriptAPI.h"
#include "ParticleLifecycle.h"

class SpectralPathEntranceVFX final : public Script
{
    DECLARE_SCRIPT(SpectralPathEntranceVFX)

public:
    explicit SpectralPathEntranceVFX(GameObject* owner);

    void Start() override;
    void OnGameStop() override;

private:
    GameObject* m_entranceEffect = nullptr;
};
