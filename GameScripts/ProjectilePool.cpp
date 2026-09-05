#include "pch.h"
#include "ProjectilePool.h"

#include "ProjectileBase.h"

namespace
{
	std::unique_ptr<Script> createProjectilePool(GameObject* owner)
	{
		return std::make_unique<ProjectilePool>(owner);
	}

	const bool s_registeredProjectilePool =
		(::registerScript("ProjectilePool", &createProjectilePool), true);
	const bool s_registeredArrowPool =
		(::registerScript("ArrowPool", &createProjectilePool), true);
}

IMPLEMENT_SCRIPT_FIELDS(ProjectilePool,
	SERIALIZED_INT(m_maxProjectiles, "Max Projectiles"),
	SERIALIZED_INT(m_legacyMaxArrows, "Max Arrows"),
	SERIALIZED_ASSET_REF(m_projectilePrefab, "Projectile Prefab", AssetType::PREFAB),
	SERIALIZED_STRING(m_legacyPrefabPath, "Arrow Prefab path")
)

ProjectilePool::ProjectilePool(GameObject* owner)
	: Script(owner)
{
}

void ProjectilePool::Start()
{
	m_projectiles.clear();
}

int ProjectilePool::resolveMaxProjectiles() const
{
	return m_maxProjectiles > m_legacyMaxArrows ? m_maxProjectiles : m_legacyMaxArrows;
}

bool ProjectilePool::createProjectile()
{
	if (!m_projectilePrefab.m_id.isValid())
	{
		Debug::error(
			"[ProjectilePool] '%s' has no projectile prefab assigned. Set Projectile Prefab to LyrielArrow (not the VFX remake).",
			GameObjectAPI::getName(getOwner()));
		return false;
	}

	// The Lyriel VFX remake is not a safe pooled projectile; the original arrow prefab is.
	if (m_projectilePrefab.m_id.m_uid == 14628721139268362793ULL)
	{
		Debug::error(
			"[ProjectilePool] '%s' has the Lyriel VFX remake as Projectile Prefab. Assign Assets/Models/Weapons/Arrow/LyrielArrow.prefab.",
			GameObjectAPI::getName(getOwner()));
		return false;
	}

	GameObject* projectileObject = GameObjectAPI::instantiatePrefab(
		m_projectilePrefab.m_id,
		Vector3::Zero,
		Vector3::Zero,
		getOwner()
	);

	if (!projectileObject)
	{
		Debug::error(
			"[ProjectilePool] '%s' failed to instantiate the assigned projectile prefab.",
			GameObjectAPI::getName(getOwner()));
		return false;
	}

	ProjectileBase* projectile = GameObjectAPI::findScript<ProjectileBase>(projectileObject);

	if (!projectile)
	{
		GameObjectAPI::setActive(projectileObject, false);
		return false;
	}

	projectile->setPool(this);
	projectile->setProjectileOwnerTransform(GameObjectAPI::getTransform(getOwner()));
	projectile->resetProjectile();

	m_projectiles.push_back(projectile);
	return true;
}

ProjectileBase* ProjectilePool::acquireProjectile()
{
	for (ProjectileBase* projectile : m_projectiles)
	{
		if (projectile && !projectile->isInUse())
		{
			return projectile;
		}
	}

	if (static_cast<int>(m_projectiles.size()) >= resolveMaxProjectiles())
	{
		return nullptr;
	}

	if (!createProjectile())
	{
		return nullptr;
	}

	return m_projectiles.back();
}

void ProjectilePool::releaseProjectile(ProjectileBase* projectile)
{
	if (!projectile)
	{
		return;
	}

	projectile->resetProjectile();
}
