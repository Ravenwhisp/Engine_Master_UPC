#include "Globals.h"

#include "QuadNode.h"
#include "Quadtree.h"

#include "GameObject.h"
#include "MeshRenderer.h"

#include <algorithm>

QuadNode::QuadNode(
    const BoundingRect& bounds,
    UINT depth,
    Quadtree& tree,
    QuadNode* parent)
    :
    m_bounds(bounds),
    m_depth(depth),
    m_tree(tree),
    m_parent(parent)
{
}

void QuadNode::insert(GameObject& object)
{
    auto* model = object.GetComponentAs<MeshRenderer>(ComponentType::MODEL);
    if (!model) return;

    const auto& box = model->getBoundingBox();

    if (!m_bounds.containsFully(box))
        return;

    if (!isLeaf())
    {
        if (QuadNode* child = findBestFitChild(box))
        {
            child->insert(object);
            return;
        }
    }

    m_objects.push_back(&object);
    m_tree.m_objectLocationMap[&object] = this;

    if (isLeaf() &&
        m_objects.size() > Quadtree::MAX_OBJECTS &&
        m_depth < Quadtree::MAX_DEPTH)
    {
        subdivide();
    }

    markDirty();
}

void QuadNode::refit(GameObject& object)
{
    auto* model = object.GetComponentAs<MeshRenderer>(ComponentType::MODEL);
    if (!model) return;

    const auto& box = model->getBoundingBox();

    if (!m_bounds.containsFully(box))
    {
        QuadNode* target = findContainingAncestor(box);

        if (!target)
            target = &m_tree.getRoot();

        remove(object);
        target->insert(object);

        return;
    }

    if (!isLeaf())
    {
        if (QuadNode* child = findBestFitChild(box))
        {
            remove(object);
            child->insert(object);
            return;
        }
    }

    markDirty();
}

void QuadNode::remove(GameObject& object)
{
    auto it = std::find(m_objects.begin(), m_objects.end(), &object);

    if (it == m_objects.end())
        return;

    m_objects.erase(it);

    m_tree.m_objectLocationMap.erase(&object);

    markDirty();
}

void QuadNode::subdivide()
{
    const float halfWidth = m_bounds.width * 0.5f;
    const float halfHeight = m_bounds.height * 0.5f;

    m_children[TOP_LEFT] = std::make_unique<QuadNode>(
        BoundingRect(m_bounds.x, m_bounds.y, halfWidth, halfHeight),
        m_depth + 1, m_tree, this);

    m_children[TOP_RIGHT] = std::make_unique<QuadNode>(
        BoundingRect(m_bounds.x + halfWidth, m_bounds.y, halfWidth, halfHeight),
        m_depth + 1, m_tree, this);

    m_children[BOTTOM_LEFT] = std::make_unique<QuadNode>(
        BoundingRect(m_bounds.x, m_bounds.y + halfHeight, halfWidth, halfHeight),
        m_depth + 1, m_tree, this);

    m_children[BOTTOM_RIGHT] = std::make_unique<QuadNode>(
        BoundingRect(m_bounds.x + halfWidth, m_bounds.y + halfHeight, halfWidth, halfHeight),
        m_depth + 1, m_tree, this);

    auto oldObjects = std::move(m_objects);

    for (GameObject* obj : oldObjects)
        insert(*obj);
}

bool QuadNode::canMerge() const
{
    if (isLeaf())
        return false;

    int total = (int)m_objects.size();

    for (const auto& child : m_children)
    {
        if (!child->isLeaf())
            return false;

        total += (int)child->m_objects.size();
    }

    return total <= Quadtree::MAX_OBJECTS;
}

void QuadNode::merge()
{
    for (auto& child : m_children)
    {
        for (GameObject* obj : child->m_objects)
        {
            m_objects.push_back(obj);
            m_tree.m_objectLocationMap[obj] = this;
        }

        child->m_objects.clear();
        child.reset();
    }
}

void QuadNode::markDirty()
{
    if (m_dirty)
        return;

    m_dirty = true;

    m_tree.registerDirtyNode(this);

    if (m_parent)
        m_parent->markDirty();
}

bool QuadNode::intersects(const Engine::Frustum& frustum, const BoundingRect& rectangle) const
{
    float fMinX = frustum.m_points[0].x;
    float fMaxX = frustum.m_points[0].x;
    float fMinZ = frustum.m_points[0].z;
    float fMaxZ = frustum.m_points[0].z;

    for (int i = 1; i < 8; ++i)
    {
        fMinX = std::min(fMinX, frustum.m_points[i].x);
        fMaxX = std::max(fMaxX, frustum.m_points[i].x);

        fMinZ = std::min(fMinZ, frustum.m_points[i].z);
        fMaxZ = std::max(fMaxZ, frustum.m_points[i].z);
    }

    float nMinX = rectangle.minX();
    float nMaxX = rectangle.maxX();
    float nMinZ = rectangle.minZ();
    float nMaxZ = rectangle.maxZ();

    bool overlapX = !(nMaxX < fMinX || nMinX > fMaxX);
    bool overlapZ = !(nMaxZ < fMinZ || nMinZ > fMaxZ);

    return overlapX && overlapZ;
}

void QuadNode::gatherObjects(const Engine::Frustum& frustum, std::vector<GameObject*>& out) const
{
    if (!intersects(frustum, m_bounds))
    {
        m_bounds.m_debugIsCulled = true;
        return;
    }

    m_bounds.m_debugIsCulled = false;

    for (GameObject* obj : m_objects)
    {
        auto* model = obj->GetComponentAs<MeshRenderer>(ComponentType::MODEL);

        if (!model || !obj->GetActive())
            continue;

        if (model->getBoundingBox().test(frustum))
            out.push_back(obj);
    }

    if (isLeaf())
        return;

    for (const auto& child : m_children)
        child->gatherObjects(frustum, out);
}

void QuadNode::gatherRectangles(std::vector<BoundingRect>& out) const
{
    out.push_back(m_bounds);

    for (const auto& child : m_children)
    {
        if (child)
            child->gatherRectangles(out);
    }
}

QuadNode* QuadNode::findBestFitChild(const Engine::BoundingBox& box) const
{
    for (const auto& child : m_children)
    {
        if (child && child->m_bounds.containsFully(box))
            return child.get();
    }

    return nullptr;
}

QuadNode* QuadNode::findContainingAncestor(const Engine::BoundingBox& box)
{
    QuadNode* node = this;

    while (node && !node->m_bounds.containsFully(box))
        node = node->m_parent;

    return node;
}