#pragma once
#include "Component.h"
#include "Lights.h"

#include <rapidjson/document.h>


class LightComponent final : public Component
{
public:
    LightComponent(UID id, GameObject* owner);

    const LightData& getData() const { return m_data; }
    LightData& editData() { return m_data; }

    void setTypeDirectional();
    void setTypePoint(float radius);
    void setTypeSpot(float radius, float innerAngleDegrees, float outerAngleDegrees);

    void sanitize();

    void drawUi() override;
    void onTransformChange() override {}

    bool isDebugDrawEnabled() const { return m_debugDrawEnabled; }
    bool isDebugDrawDepthEnabled() const { return m_debugDrawDepthEnabled; }

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentInfo) override;

private:
    LightData m_data{};

    bool m_debugDrawEnabled = false;
    bool m_debugDrawDepthEnabled = true;
};