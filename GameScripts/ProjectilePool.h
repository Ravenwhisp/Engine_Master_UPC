#pragma once

#include "ScriptAPI.h"

#include <vector>

class ProjectileBase;

class ProjectilePool : public Script
{
	DECLARE_SCRIPT(ProjectilePool)

public:
	explicit ProjectilePool(GameObject* owner);

	void Start() override;

	FieldList getExposedFields() const override;

	ProjectileBase* acquireProjectile();
	void releaseProjectile(ProjectileBase* projectile);

private:
	bool createProjectile();

public:
	int m_maxProjectiles = 5;
	PrefabRef m_projectilePrefab;

private:
	std::vector<ProjectileBase*> m_projectiles;
};