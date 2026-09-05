#pragma once

#include "ScriptAPI.h"

#include <string>
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
	int resolveMaxProjectiles() const;

public:
	int m_maxProjectiles = 5;
	int m_legacyMaxArrows = 0;
	PrefabRef m_projectilePrefab;
	std::string m_legacyPrefabPath;

private:
	std::vector<ProjectileBase*> m_projectiles;
};
