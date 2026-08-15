#include "pch.h"
#include "ProjectilePool.h"

#include "ProjectileBase.h"

IMPLEMENT_SCRIPT_FIELDS(ProjectilePool,
	SERIALIZED_INT(m_maxProjectiles, "Max Projectiles"),
	SERIALIZED_ASSET_REF(m_projectilePrefab, "Projectile Prefab", AssetType::PREFAB)
)

ProjectilePool::ProjectilePool(GameObject* owner)
	: Script(owner)
{
}

void ProjectilePool::Start()
{
	m_projectiles.clear();

	for (int i = 0; i < m_maxProjectiles; ++i)
	{
		if (!createProjectile())
		{
			Debug::log("[ProjectilePoolBase] Failed to create projectile %d", i);
			break;
		}
	}
}

bool ProjectilePool::createProjectile()
{
	if (!m_projectilePrefab.m_id.isValid())
	{
		return false;
	}

	GameObject* projectileObject = GameObjectAPI::instantiatePrefab(
		m_projectilePrefab.m_id,
		Vector3::Zero,
		Vector3::Zero,
		nullptr
	);

	if (!projectileObject)
	{
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

	return nullptr;
}

void ProjectilePool::releaseProjectile(ProjectileBase* projectile)
{
	if (!projectile)
	{
		return;
	}

	projectile->resetProjectile();
}

IMPLEMENT_SCRIPT(ProjectilePool)