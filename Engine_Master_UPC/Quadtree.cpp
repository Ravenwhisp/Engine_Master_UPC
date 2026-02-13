#include "Globals.h"
#include "Quadtree.h"

Quadtree::Quadtree(const RectangleData& worldBounds)
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
        return;

    QuadNode* node = it->second;
    node->remove(object);
    m_objectLocationMap.erase(it);
}

void Quadtree::move(GameObject& object)
{
    remove(object);
    insert(object);
}

std::vector<GameObject*> Quadtree::getObjects(BoundingFrustum& frustum) const
{
    std::vector<GameObject*> result;
    m_root->gatherObjects(frustum, result);
    return result;
}

std::vector<RectangleData> Quadtree::getQuadrants() const
{
    std::vector<RectangleData> result;
    m_root->gatherRectangles(result);
    return result;
}
