#pragma once
#include "Component.h"

class IArchive;

class OcclusionTargetComponent : public Component
{
public:
    OcclusionTargetComponent(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    void drawUi() override;
    void serialize(IArchive& archive) override;

    float getBubbleScale() const { return m_bubbleScale; }
    float getBubbleSoftness() const { return m_bubbleSoftness; }
    float getBodyHeight() const { return m_bodyHeight; }
    float getAnchorHeight() const { return m_anchorHeight; }
    float getOcclusionMargin() const { return m_occlusionMargin; }

private:
    float m_bubbleScale = 1.35f;
    float m_bubbleSoftness = 0.35f;
    float m_bodyHeight = 0.0f; // Root-local units; zero derives height from the body mesh.
    float m_anchorHeight = 0.55f; // Fraction of body height, measured above the root.
    float m_occlusionMargin = 0.025f; // Fraction of body height along the view direction.
};
