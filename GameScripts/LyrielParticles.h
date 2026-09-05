#pragma once
#include <Script.h>
#include "ScriptAPI.h"
#include "ParticleLifecycle.h"

#include <string>

class LyrielParticles : public Script
{
	DECLARE_SCRIPT(LyrielParticles)

public:
	explicit LyrielParticles(GameObject* owner);

	void Start() override;
	void Update() override;
	void OnGameStop() override;

	FieldList getExposedFields() const override;

	void SetDashActive();
	void SetDashInactive();

	void SetChargeActive();
	void SetChargeInactive();

	void playHitFlash(const Vector3& position);

	ComponentRef<Transform> m_dashTrail;
	PrefabRef m_chargeGlowPrefab;
	PrefabRef m_dashParticlePrefab;
	PrefabRef m_hitFlashPrefab;

	std::string m_chargeGlowPath = "Assets/Prefabs/Particles/Lyriel/LyrielChargeGlow.prefab";
	std::string m_dashParticlePath = "Assets/Prefabs/Particles/Lyriel/LyrielDashParticles.prefab";
	std::string m_hitFlashPath = "Assets/Prefabs/Particles/Lyriel/LyrielHitFlash.prefab";
	std::string m_bowAnchorName = "ArrowSpawn";

private:
	Transform* getTransform(ComponentRef<Transform> controller);
	Transform* findBowTransform() const;
	void syncActiveParticles();

	Transform* m_dashTrailController = nullptr;
	GameObject* m_chargeGlowInstance = nullptr;
	GameObject* m_dashParticleInstance = nullptr;
	bool m_chargeGlowActive = false;
	bool m_dashParticleActive = false;
	ParticleLifecycle::TimedParticleTracker m_timedOneShots;
};
