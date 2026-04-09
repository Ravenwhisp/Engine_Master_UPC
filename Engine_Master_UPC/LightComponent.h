#pragma once
#include "Component.h"
#include "Lights.h"
#include "IDebugDrawable.h"
    
class LightComponent final : public Component
{
public:
    LightComponent(UID id, GameObject* owner);
    virtual std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    const LightData& getData() const { return m_data; }
    LightData& editData() { return m_data; }

    void setTypeDirectional();
    void setTypePoint(float radius);
    void setTypeSpot(float radius, float innerAngleDegrees, float outerAngleDegrees);

    void sanitize();

    void drawUi() override;
    void onTransformChange() override {}

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentInfo) override;

    void debugDraw() override;
private:
    LightData m_data{};
};