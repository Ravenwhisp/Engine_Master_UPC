#pragma once

#include "ScriptAPI.h"
#include <unordered_set>

class SpikeTrap : public Script
{
    DECLARE_SCRIPT(SpikeTrap)

public:
    explicit SpikeTrap(GameObject* owner);

    void Start() override;
    void Update() override;

    enum TrapState
    {
        WAIT,
        ACTIVE
    };
    
    ScriptFieldList getExposedFields() const override;

    float a_duration = 2.0;
	float p_duration = 1.0;

	float currentTime = 0.0f;

	float trapDamage = 20.0f;

	bool alternativeMode = false;

    /*ScriptComponentRef<Transform> m_normalSpike;
    ScriptComponentRef<Transform> m_spectralSpike;*/

	Vector3 normalSpikePosition = Vector3(0.0f, -1.0f, 0.0f);
	Vector3 spectralSpikePosition = Vector3(0.0f, -1.0f, 0.0f);

    float m_xWidth = 2.0f;
    float m_zWidth = 2.0f;

	GameObject* owner = nullptr;
	Transform* ownerTransform = nullptr;

    Transform* m_normalSpike;
	Transform* m_spectralSpike;


    //This will make posible use different height for the spikes, which let us use different models. If finally we use animations this will disspaear.
	float startPositionY = -1.0f;
	float waitPositionY = -0.7f;
	float activePositionY = 0.0f;

private:
    
    bool containsPoint(const Vector3& triggerCenter, const Vector3& point) const;
    void TrapLoop();

	void damagePlayer(GameObject* player);

    void triggerBoxDamage();

	int spikeType = 0; // 0 for normal, 1 for spectral

    std::unordered_set<GameObject*> damagedPlayers;

	TrapState state = WAIT;
};
