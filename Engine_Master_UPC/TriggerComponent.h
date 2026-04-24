#pragma once

#include "Component.h"
#include "TriggerShape.h"
#include "BoundingBox.h"

class TriggerComponent : public Component
{
public:
    TriggerComponent(UID id, GameObject* gameObject);
    ~TriggerComponent() override = default;

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    void drawUi() override;
    void debugDraw() override;

    void onTransformChange() override { m_boundsDirty = true; };

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentValue) override;

    TriggerShape getShape() const { return m_shape; };

    const Vector3& getCenter() const { return m_center; };
    const Vector3& getSize() const { return m_size; };

    void setCenter(const Vector3& center) ;
    void setSize(const Vector3& size);

    const Engine::BoundingBox& getWorldAABB() const;

private:
    void recalculateWorldAABB();
    bool isValidSize() const;

private:
    TriggerShape m_shape = TriggerShape::Box;

    Vector3 m_center = Vector3::Zero;
    Vector3 m_size = Vector3::One;

    mutable Engine::BoundingBox m_worldAABB;
    mutable bool m_boundsDirty = true;
};