#pragma once
#include <Script.h>
#include "ScriptAPI.h"
#include "ParticleLifecycle.h"

#include <string>

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

	void playHitFlash(const Vector3& position);
	void playChargedHitFlash(const Vector3& position);

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

	Transform* m_dashTrailController = nullptr;
	Transform* m_scytheTrailController = nullptr;
	ParticleLifecycle::TimedParticleTracker m_timedOneShots;
};
