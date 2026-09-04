#pragma once
#include <Script.h>
#include "ScriptAPI.h"

class DeathParticles : public Script
{
	DECLARE_SCRIPT(DeathParticles)
public:

	explicit DeathParticles(GameObject* owner);

	void Update() override;

	ComponentRef<Transform> m_dashTrail;
	ComponentRef<Transform> m_scytheTrail;
	PrefabRef m_tauntParticle;

	GameObject* m_activeTauntParticle = nullptr;
	float m_tauntParticleLifetime = 0.0f;

	Transform* m_dashTrailController = nullptr;
	Transform* m_scytheTrailController = nullptr;

	FieldList getExposedFields() const override;

	void SetDashActive();
	void SetDashInactive();

	void SetScytheActive();
	void SetScytheInactive();

	void SetTauntActive(const Vector3& direction);
	void SetTauntInactive();

private:

	Transform* getTransform(ComponentRef<Transform> controller);
};

