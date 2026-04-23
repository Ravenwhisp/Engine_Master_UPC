#include "pch.h"
#include "ArrowPool.h"
#include "LyrielArrowProjectile.h"

IMPLEMENT_SCRIPT_FIELDS(ArrowPool,
    SERIALIZED_INT(m_maxArrows, "Max Arrows"),
    SERIALIZED_STRING(m_arrowPrefabPath, "Arrow Prefab Path")
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
    if (m_arrowPrefabPath.empty())
    {
        return false;
    }

    GameObject* arrowObject = GameObjectAPI::instantiatePrefab(m_arrowPrefabPath.c_str(), Vector3::Zero, Vector3::Zero, nullptr);   
    if (arrowObject == nullptr)
    {
        return false;
    }

    Script* script = GameObjectAPI::getScript(arrowObject, "LyrielArrowProjectile");
    if (script == nullptr)
    {
        GameObjectAPI::setActive(arrowObject, false);
        return false;
    }
    LyrielArrowProjectile* arrow = static_cast<LyrielArrowProjectile*>(script);

    arrow->setPool(this);
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