#pragma once

#include "ScriptAPI.h"

class ProjectilePool;

class ProjectileBase : public Script
{
	DECLARE_SCRIPT(ProjectileBase)

public:
	explicit ProjectileBase(GameObject* owner);

	bool isInUse() const;

	void setPool(ProjectilePool* pool);

	void setProjectileOwnerTransform(Transform* ownerTransform);
	Transform* getProjectileOwnerTransform() const;

	virtual void resetProjectile();
	virtual void returnToPool();

protected:
	ProjectilePool* m_pool = nullptr;
	Transform* m_projectileOwnerTransform = nullptr;

	bool m_inUse = false;
};