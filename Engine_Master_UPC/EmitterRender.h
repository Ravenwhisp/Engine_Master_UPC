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
    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& moduleInfo) override;

    RenderMode getRenderMode() const { return m_renderMode; }

private:
    RenderMode m_renderMode = RenderMode::BILLBOARD;
};