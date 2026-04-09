#pragma once

#include "ScriptAPI.h"

class ProjectileController : public Script
{
    DECLARE_SCRIPT(ProjectileController)

public:
    explicit ProjectileController(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

    float m_minSpeed = 1; //
    float m_maxSpeed = 5; // m_minSpeed <= m_maxSpeed!

    float m_scale = 0; // determines the amount of speed that the projectile object will start with ([0,1], between minSpeed and maxSpeed)

    ScriptComponentRef<Transform> m_target; // the object to hit (will ignore the rest)

    std::string m_prefabToInstantiate = "";

private:

    bool hit(); // temporary, to determine if we have hit our target

    Transform* m_objectTransform;

    float m_speed;
    bool m_canPierce;

    bool pressed = false;
};