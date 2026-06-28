#pragma once

#include "Component.h"
#include "TriggerShape.h"
#include "TriggerDebugDrawMode.h"
#include "BoundingBox.h"

class MeshRenderer;

class TriggerComponent : public Component
{
public:
    TriggerComponent(UID id, GameObject* gameObject);
    ~TriggerComponent() override = default;

    bool init() override;

    void drawUi() override;
    void debugDraw() override;

    void onTransformChange() override { m_boundsDirty = true; };

    void serialize(IArchive& archive) override;

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

#pragma region Shape data
    TriggerShape getShape() const { return m_shape; };

    const Vector3& getCenter() const { return m_center; };
    const Vector3& getSize() const { return m_size; };

    void setCenter(const Vector3& center);
    void setSize(const Vector3& size);
#pragma endregion

#pragma region Bounds
    Engine::BoundingBox& getWorldBox();
    Engine::BoundingBox& getWorldAABB();
#pragma endregion


private:

#pragma region Bounds calculation
    void recalculateWorldBounds();
    bool isValidSize() const;
#pragma endregion

#pragma region Defaul Model bounds
    bool setDefaultBoundsFromModel();
    void includeHierarchyModelBounds(GameObject* object, const Matrix& triggerWorldInverse, Vector3& boundsMin, Vector3& boundsMax, bool& hasBounds);
    void includeMeshRendererBounds(MeshRenderer* meshRenderer, const Matrix& triggerWorldInverse, Vector3& boundsMin, Vector3& boundsMax, bool& hasBounds);
#pragma endregion

private:
    TriggerShape m_shape = TriggerShape::Box;

    Vector3 m_center = Vector3::Zero;
    Vector3 m_size = Vector3::One;

    Engine::BoundingBox m_worldBox;
    Engine::BoundingBox m_worldAABB;
    bool m_boundsDirty = true;


    TriggerDebugDrawMode m_debugDrawMode = TriggerDebugDrawMode::RotatedBox;

    bool m_setDefaultBoundsOnInit = true;

};