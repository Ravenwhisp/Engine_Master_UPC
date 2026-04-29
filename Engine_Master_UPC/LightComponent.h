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

#pragma region Serialization
    template <class Archive>
    void save(Archive& ar) const
    {
        ar(cereal::base_class<Component>(this),m_data);
    }

    template<class Archive>
    static void load_and_construct(
        Archive& ar,
        cereal::construct<LightComponent>& construct)
    {
        UID           id;
        ComponentType type;
        bool          active;
        ar(id, type, active);

        construct(id, nullptr);
        construct->setActive(active);

        ar(construct->m_data);
    }

private:
    LightData m_data{};
};


CEREAL_REGISTER_TYPE(LightComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, LightComponent)



