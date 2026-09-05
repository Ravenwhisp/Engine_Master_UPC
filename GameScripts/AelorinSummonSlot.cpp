#include "pch.h"
#include "AelorinSummonSlot.h"

#include "EnemyDamageable.h"

IMPLEMENT_SCRIPT_FIELDS(AelorinSummonSlot,
	SERIALIZED_ASSET_REF(m_enemyPrefab, "Enemy Prefab", AssetType::PREFAB)
)

AelorinSummonSlot::AelorinSummonSlot(GameObject* owner)
	: Script(owner)
{
}

bool AelorinSummonSlot::hasLivingEnemy() const
{
	Transform* slotTransform = GameObjectAPI::getTransform(getOwner());
	if (!slotTransform)
	{
		return false;
	}

	const int childCount = TransformAPI::getChildCount(slotTransform);

	for (int i = 0; i < childCount; ++i)
	{
		Transform* childTransform = TransformAPI::getChild(slotTransform, i);
		if (!childTransform)
		{
			continue;
		}

		GameObject* enemyObject = ComponentAPI::getOwner(childTransform);
		if (!enemyObject)
		{
			continue;
		}

		EnemyDamageable* damageable = GameObjectAPI::findScript<EnemyDamageable>(enemyObject);
		if (damageable && !damageable->isDead())
		{
			return true;
		}
	}

	return false;
}

GameObject* AelorinSummonSlot::spawnEnemy()
{
	if (hasLivingEnemy())
	{
		return nullptr;
	}

	Transform* slotTransform = GameObjectAPI::getTransform(getOwner());
	if (!slotTransform)
	{
		return nullptr;
	}

	const Vector3 spawnPosition = TransformAPI::getGlobalPosition(slotTransform);
	GameObject* enemy = GameObjectAPI::instantiatePrefab(m_enemyPrefab.m_id, spawnPosition, Vector3::Zero, getOwner());

	if (!enemy)
	{
		Debug::warn("[AelorinSummonSlot] Failed to spawn enemy on %s", GameObjectAPI::getName(getOwner()));
		return nullptr;
	}

	return enemy;
}

IMPLEMENT_SCRIPT(AelorinSummonSlot)