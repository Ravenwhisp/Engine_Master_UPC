#pragma once
#include <Script.h>
#include "ScriptAPI.h"
#include "ParticleLifecycle.h"
#include "DeathTauntChains.h"

#include <string>
#include <vector>

class DeathParticles : public Script
{
	DECLARE_SCRIPT(DeathParticles)

public:
	explicit DeathParticles(GameObject* owner);

	void Start() override;
	void Update() override;
	void OnGameStop() override;

	FieldList getExposedFields() const override;

	void SetDashActive();
	void SetDashInactive();

	void SetScytheActive();
	void SetScytheInactive();

	void SetChargeActive();
	void SetChargeInactive();

	void SetTauntActive(const Vector3& direction);
	void SetTauntInactive();

	void playHitFlash(const Vector3& position, GameObject* target);
	void playChargedHitFlash(const Vector3& position, GameObject* target);

	// Taunt chain (visuals live in DeathTauntChains; prefabs and settings are the fields below)
	void launchTauntChains(const std::vector<GameObject*>& targets, float travelTime);
	void cancelTauntChains();
	void notifyTauntChainPullStarted(GameObject* enemy);
	void notifyTauntChainPullFinished(GameObject* enemy);
	float getTauntChainTravelTime() const { return m_tauntChainTravelTime; }

	ComponentRef<Transform> m_dashTrail;
	ComponentRef<Transform> m_scytheTrail;

	PrefabRef m_tauntParticle;
	PrefabRef m_dashParticlePrefab;
	PrefabRef m_chargeGlowPrefab;
	PrefabRef m_hitFlashPrefab;
	PrefabRef m_chargedHitFlashPrefab;

	std::string m_tauntParticlePath = "Assets/Prefabs/Particles/Death/DeathTauntEffect.prefab";
	std::string m_dashParticlePath = "Assets/Prefabs/Particles/Death/DeathDashParticles.prefab";
	std::string m_chargeGlowPath = "Assets/Prefabs/Particles/Death/DeathChargeGlow.prefab";
	std::string m_hitFlashPath = "Assets/Prefabs/Particles/Death/DeathHitFlash.prefab";
	std::string m_chargedHitFlashPath = "Assets/Prefabs/Particles/Death/DeathChargedHitFlash.prefab";
	std::string m_scytheAnchorName = "ScytheAnchor";

	PrefabRef m_tauntChainLinkPrefab; 
	PrefabRef m_tauntChainGrabBurstPrefab;
	PrefabRef m_tauntChainContactPuffPrefab;
	std::string m_tauntChainHandBone = "hand_L";
	bool m_tauntChainOnFloor = true;
	float m_tauntChainFloorHeight = 0.13f;  // height of the link centres; links are 0.2 wide, so this keeps upright ones off the ground
	float m_tauntChainStartOffset = 0.4f;
	float m_tauntChainTipHeight = 1.0f;
	float m_tauntChainContactOffset = 0.45f;
	float m_tauntChainGrabBurstDelay = 0.1f;
	float m_tauntChainLinkSpacing = 0.22f;  
	float m_tauntChainLinkScale = 1.0f;
	float m_tauntChainTipLinkScale = 1.25f;
	float m_tauntChainMaxLength = 7.0f;
	int m_tauntChainMaxChains = 3;
	float m_tauntChainTravelTime = 0.18f;
	float m_tauntChainMinLatchTime = 0.35f;
	float m_tauntChainMaxLatchTime = 1.5f;
	float m_tauntChainRetractTime = 0.2f;
	float m_tauntChainWhip = 0.35f;         // sideways wave while flying out
	float m_tauntChainTension = 0.05f;      // shake while pulling

private:
	Transform* getTransform(ComponentRef<Transform> controller);
	Transform* findScytheTransform() const;
	void ensureTauntParticle(const Vector3& position, const Vector3& rotation);
	void syncActiveParticles();

	GameObject* m_activeTauntParticle = nullptr;
	GameObject* m_dashParticleInstance = nullptr;
	GameObject* m_chargeGlowInstance = nullptr;
	float m_tauntParticleLifetime = 0.0f;
	bool m_tauntParticleActive = false;
	bool m_dashParticleActive = false;
	bool m_chargeGlowActive = false;

	DeathTauntChains m_tauntChains{ *this };

	Transform* m_dashTrailController = nullptr;
	Transform* m_scytheTrailController = nullptr;
	ParticleLifecycle::TimedParticleTracker m_timedOneShots;
};
