#include "pch.h"
#include "ArrowPool.h"
#include "LyrielArrowProjectile.h"

IMPLEMENT_SCRIPT_FIELDS(ArrowPool,
    SERIALIZED_INT(m_maxArrows, "Max Arrows"),
    SERIALIZED_ASSET_REF(m_arrowPrefab, "Arrow Prefab", AssetType::PREFAB)
)

ArrowPool::ArrowPool(GameObject* owner)
    : Script(owner)
{
}

void ArrowPool::Start()
{
    m_arrows.clear();

    for (int i = 0; i < m_maxArrows; ++i)
    {
        if (!createArrow())
        {
            Debug::log("[ArrowPool] Failed to create arrow %d", i);
            break;
        }
    }
}

bool ArrowPool::createArrow()
{
    if (m_arrowPrefab.m_ref.isValid())
    {
        return false;
    }

    GameObject* arrowObject = GameObjectAPI::instantiatePrefab(m_arrowPrefab.m_ref, Vector3::Zero, Vector3::Zero, nullptr);   
    if (arrowObject == nullptr)
    {
        return false;
    }

    LyrielArrowProjectile* arrow = GameObjectAPI::findScript<LyrielArrowProjectile>(arrowObject);
    if (arrow == nullptr)
    {
        GameObjectAPI::setActive(arrowObject, false);
        return false;
    }

    arrow->setPool(this);
    arrow->setArrowOwnerTransform(GameObjectAPI::getTransform(getOwner()));
    arrow->resetProjectile();

    m_arrows.push_back(arrow);
    return true;
}

LyrielArrowProjectile* ArrowPool::acquireArrow()
{
    for (LyrielArrowProjectile* arrow : m_arrows)
    {
        if (arrow != nullptr && !arrow->isInUse())
        {
            return arrow;
        }
    }

    return nullptr;
}

void ArrowPool::releaseArrow(LyrielArrowProjectile* arrow)
{
    if (arrow == nullptr)
    {
        return;
    }

    arrow->resetProjectile();
}

IMPLEMENT_SCRIPT(ArrowPool)