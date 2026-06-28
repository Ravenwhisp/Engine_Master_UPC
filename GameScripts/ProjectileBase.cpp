#include "pch.h"
#include "ProjectileBase.h"

#include "ProjectilePool.h"

ProjectileBase::ProjectileBase(GameObject* owner)
	: Script(owner)
{
}

bool ProjectileBase::isInUse() const
{
	return m_inUse;
}

void ProjectileBase::setPool(ProjectilePool* pool)
{
	m_pool = pool;
}

void ProjectileBase::setProjectileOwnerTransform(Transform* ownerTransform)
{
	m_projectileOwnerTransform = ownerTransform;
}

Transform* ProjectileBase::getProjectileOwnerTransform() const
{
	return m_projectileOwnerTransform;
}

void ProjectileBase::resetProjectile()
{
	m_inUse = false;
	GameObjectAPI::setActive(getOwner(), false);
}

void ProjectileBase::returnToPool()
{
	if (!m_pool)
	{
		resetProjectile();
		return;
	}

	m_pool->releaseProjectile(this);
}

IMPLEMENT_SCRIPT(ProjectileBase)