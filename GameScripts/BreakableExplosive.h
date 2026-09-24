#pragma once

#include "ScriptAPI.h"
#include "BreakableObject.h"

#include <vector>

class Transform2D;

class BreakableExplosive : public BreakableObject
{
    DECLARE_SCRIPT(BreakableExplosive)

public:
    explicit BreakableExplosive(GameObject* owner);

    void Start() override;
    void Update() override;
    void drawGizmo() override;

    FieldList getExposedFields() const override;

    bool canBeTargetedDuringCombat() const override { return true; }

private:
    void onBreak() override;

    void setupTelegraph();
    void updateTelegraph(float deltaTime);
    bool isAnyPlayerWithin(float distance) const;
    void applyTelegraphAlpha(float alpha);
    void hideTelegraphImmediately();

    Transform* m_ownerTransform = nullptr;
    Transform* m_telegraphCanvasTransform = nullptr;
    Transform2D* m_telegraphVisualTransform = nullptr;
    GameObject* m_telegraphCanvasObject = nullptr;

    std::vector<GameObject*> m_players;
    float m_currentTelegraphAlpha = 0.0f;
    bool m_telegraphShouldShow = false;

public:
    float m_explosionRadius = 5.0f;
    float m_explosionDamage = 30.0f;

    float m_telegraphRevealDistance = 8.5f;
    // Slightly larger than the reveal distance to prevent boundary flicker.
    float m_telegraphHideDistance = 9.5f;
    float m_telegraphFadeDuration = 0.2f;
    float m_telegraphMaxAlpha = 0.30f;
    float m_telegraphHeightOffset = 0.05f;

    ComponentRef<Transform> m_telegraphCanvas;
    ComponentRef<Transform2D> m_telegraphVisual;
};
