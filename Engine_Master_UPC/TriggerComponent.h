#pragma once

#include "Component.h"
#include "TriggerShape.h"
#include "TriggerDebugDrawMode.h"
#include "BoundingBox.h"

class TriggerComponent : public Component
{
public:
    TriggerComponent(UID id, GameObject* gameObject);
    ~TriggerComponent() override = default;

    bool init() override;
    bool cleanUp() override;

    void drawUi() override;
    void debugDraw() override;

    void onTransformChange() override { m_boundsDirty = true; };

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentValue) override;

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    TriggerShape getShape() const { return m_shape; };

    const Vector3& getCenter() const { return m_center; };
    const Vector3& getSize() const { return m_size; };

    void setCenter(const Vector3& center) ;
    void setSize(const Vector3& size);

    Engine::BoundingBox& getWorldBox();
    Engine::BoundingBox& getWorldAABB();

private:
    void recalculateWorldBounds();
    bool isValidSize() const;

private:
    TriggerShape m_shape = TriggerShape::Box;

    Vector3 m_center = Vector3::Zero;
    Vector3 m_size = Vector3::One;

    Engine::BoundingBox m_worldBox;
    Engine::BoundingBox m_worldAABB;

    TriggerDebugDrawMode m_debugDrawMode = TriggerDebugDrawMode::RotatedBox;

    bool m_boundsDirty = true;
};