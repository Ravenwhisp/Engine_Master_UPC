#pragma once

#include "ScriptAPI.h"

class Pickup : public Script
{
    DECLARE_SCRIPT(Pickup)

public:
    explicit Pickup(GameObject* owner);

    void Start() override;
    void Update() override;
	void OnTriggerEnter(GameObject* gameObject) override;

    //FieldList getExposedFields() const override;
//
//public:
//    float m_idleSpeed = 0.5f;
//    float m_horizontalAmplitude = 0.1f;
//    float m_verticalAmplitude = 0.2f;
//
//protected:
//    void idleAnimation();

protected:
    Vector3 m_startPosition = Vector3::Zero;

	bool m_collected = false;
};