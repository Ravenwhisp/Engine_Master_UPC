#include "Globals.h"
#include "Quadtree.h"
#include "MeshRenderer.h"
#include "GameObject.h"
#include <algorithm>

Quadtree::Quadtree(const BoundingRect& worldBounds)
{
    m_root = std::make_unique<QuadNode>(worldBounds, 0, *this, nullptr);
}

void Quadtree::insert(GameObject& object)
{
    m_root->insert(object);
}

void Quadtree::remove(GameObject& object)
{
    auto it = m_objectLocationMap.find(&object);
    if (it == m_objectLocationMap.end())
    {
        return;
    }

    QuadNode* node = it->second;
    node->remove(object);
}

void Quadtree::move(GameObject& object)
{
    auto it = m_objectLocationMap.find(&object);

    if (it == m_objectLocationMap.end())
    {
        insert(object);
        return;
    }

    it->second->refit(object);
}


std::vector<GameObject*> Quadtree::getObjects(const Engine::Frustum* frustum) const
{
    std::vector<GameObject*> result;
    m_root->gatherObjects(frustum, result);
    return result;
}

std::vector<BoundingRect> Quadtree::getQuadrants() const
{
    std::vector<BoundingRect> result;
    m_root->gatherRectangles(result);
    return result;
}

void Quadtree::registerDirtyNode(QuadNode* node)
{
    m_dirtyNodes.push_back(node);
}

void Quadtree::resolveDirtyNodes()
{
    sort(m_dirtyNodes.begin(), m_dirtyNodes.end(),
        [](QuadNode* a, QuadNode* b)
        {
            return a->getDepth() > b->getDepth();
        });

    for (QuadNode* node : m_dirtyNodes)
    {
        if (node->canMerge())
        {
            node->merge();
        }

        node->clearDirty();
    }

    m_dirtyNodes.clear();
}

