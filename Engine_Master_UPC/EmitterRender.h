#pragma once

#include "ParticleModule.h"

class EmitterRender : public ParticleModule
{
public:
    enum class RenderMode {
        BILLBOARD = 0,
        HORIZONTAL = 1,
        VERTICAL = 2
    };

    EmitterRender() : ParticleModule(ParticleModuleType::RENDER) {}
    std::unique_ptr<ParticleModule> clone() const override { return std::make_unique<EmitterRender>(*this); }

    bool drawUi() override;
    void serialize(IArchive& archive) override;

    RenderMode getRenderMode() const { return m_renderMode; }
    uint32_t getLayer() const { return m_layer; }

private:
    RenderMode m_renderMode = RenderMode::BILLBOARD;

    uint32_t m_layer = 0; // to indicate the order which particles between overlapped emitters will be drawn in (higher => more on top)
};