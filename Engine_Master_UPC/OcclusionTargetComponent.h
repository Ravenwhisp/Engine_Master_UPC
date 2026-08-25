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

    float getRevealStrength() const { return m_revealStrength; }
    float getBubbleScale() const { return m_bubbleScale; }
    float getBubbleSoftness() const { return m_bubbleSoftness; }
    float getOccluderOpacity() const { return m_occluderOpacity; }

private:
    float m_revealStrength = 0.65f;
    float m_bubbleScale = 1.35f;
    float m_bubbleSoftness = 0.35f;
    float m_occluderOpacity = 0.65f;
};