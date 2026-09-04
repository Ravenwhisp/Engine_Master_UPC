#pragma once
#include "ScriptAPI.h"
#include <unordered_set>

class LightZoneManager;

class LightZoneTrigger : public Script
{
    DECLARE_SCRIPT(LightZoneTrigger)

public:
    explicit LightZoneTrigger(GameObject* owner);

    void Start() override;
    void Update() override;

    FieldList getExposedFields() const override;

    float m_xWidth = 2.0f;
    float m_zWidth = 2.0f;

private:
    bool containsPoint(const Vector3& center, const Vector3& point) const;

    LightZoneManager* m_manager = nullptr;
    std::unordered_set<GameObject*> m_playersInside;
    bool m_bothWereInside = false;
};
