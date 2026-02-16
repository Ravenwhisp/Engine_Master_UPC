#include "Globals.h"
#include "QuadNode.h"
#include "Quadtree.h"
#include "BasicModel.h"
#include "GameObject.h"
#include "BoundingBox.h"

QuadNode::QuadNode(const BoundingRect& bounds,
    UINT depth,
    Quadtree& tree,
    QuadNode* parent)
    : m_bounds(bounds),
    m_depth(depth),
    m_tree(tree),
    m_parent(parent)
{
}

void QuadNode::insert(GameObject& object)
{
    auto model = object.GetComponent<BasicModel>();
    if (!model) return;

    const auto& box = model->getBoundingBox();

    if (!m_bounds.containsFully(box)) return;

    if (!isLeaf())
    {
        for (auto& child : m_children)
        {
            if (child->m_bounds.containsFully(box))
            {
                child->insert(object);
                return;
            }
        }
    }

    m_objects.push_back(&object);
    m_tree.m_objectLocationMap[&object] = this;

    if (isLeaf() && m_objects.size() > Quadtree::MAX_OBJECTS && m_depth < Quadtree::MAX_DEPTH)
    {
        subdivide();
    }
}

void QuadNode::refit(GameObject& object)
{
    auto model = object.GetComponent<BasicModel>();
    if (!model)return;

    const auto& box = model->getBoundingBox();

    if (!m_bounds.containsFully(box))
    {
        QuadNode* current = m_parent;

        while (current && !current->m_bounds.containsFully(box))
        {
            current = current->m_parent;
        }


        if (!current)
        {
            current = &m_tree.getRoot();
        }

        remove(object);
        current->insert(object);
        return;
    }

    if (!isLeaf())
    {
        for (auto& child : m_children)
        {
            if (child->m_bounds.containsFully(box))
            {
                remove(object);
                child->insert(object);
                return;
            }
        }
    }
}


void QuadNode::remove(GameObject& object)
{
    auto& objs = m_objects;
    objs.erase(std::remove(objs.begin(), objs.end(), &object), objs.end());

    m_tree.m_objectLocationMap.erase(&object);

    tryMergeUpwards();
}

void QuadNode::subdivide()
{
    float halfwidth = m_bounds.width * 0.5f;
    float halfheight = m_bounds.height * 0.5f;

    m_children[TOP_LEFT] = std::make_unique<QuadNode>(
        BoundingRect(m_bounds.x, m_bounds.y, halfwidth, halfheight),
        m_depth + 1, m_tree, this);

    m_children[TOP_RIGHT] = std::make_unique<QuadNode>(
        BoundingRect(m_bounds.x + halfwidth, m_bounds.y, halfwidth, halfheight),
        m_depth + 1, m_tree, this);

    m_children[BOTTOM_LEFT] = std::make_unique<QuadNode>(
        BoundingRect(m_bounds.x, m_bounds.y + halfheight, halfwidth, halfheight),
        m_depth + 1, m_tree, this);

    m_children[BOTTOM_RIGHT] = std::make_unique<QuadNode>(
        BoundingRect(m_bounds.x + halfwidth, m_bounds.y + halfheight, halfwidth, halfheight),
        m_depth + 1, m_tree, this);

    std::vector<GameObject*> oldObjects = std::move(m_objects);
    m_objects.clear();

    for (GameObject* obj : oldObjects)
    {
        insert(*obj);
    }
}

void QuadNode::tryMergeUpwards()
{
    QuadNode* current = this;

    while (current->m_parent)
    {
        QuadNode* parent = current->m_parent;

        if (!parent->canMerge())
        {
            break;
        }

        parent->merge();
        current = parent;
    }
}

bool QuadNode::canMerge() const
{
    if (isLeaf()) 
    { 
        return false; 
    }

    int total = 0;

    for (const auto& child : m_children)
    {
        if (!child->isLeaf()) 
        { 
            return false;
        }
        total += child->m_objects.size();
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

        child.reset();
    }
}

bool QuadNode::intersects(const Engine::Frustum& frustum, const BoundingRect& rectangle, int minY, int maxY) const
{
    Vector2 center;
    center.x = rectangle.x + rectangle.width * 0.5f;
    center.y = rectangle.y + rectangle.height * 0.5f;

    Vector2 extents;
    extents.x = rectangle.width * 0.5f;
    extents.y = rectangle.height * 0.5f;

    std::vector<Vector2> frustumXZ;
    for (int i = 0; i < 8; i++) {
        frustumXZ.push_back(Vector2(frustum.m_points[i].x, frustum.m_points[i].z));
    }

    Vector2 fMin = frustumXZ[0];
    Vector2 fMax = frustumXZ[0];
    for (const auto& p : frustumXZ) {
        fMin.x = std::min(fMin.x, p.x);
        fMin.y = std::min(fMin.y, p.y);
        fMax.x = std::max(fMax.x, p.x);
        fMax.y = std::max(fMax.y, p.y);
    }

    // Compute node AABB in XZ
    Vector2 nMin = center - extents;
    Vector2 nMax = center + extents;

    // Test overlap
    bool overlapX = !(nMax.x < fMin.x || nMin.x > fMax.x);
    bool overlapZ = !(nMax.y < fMin.y || nMin.y > fMax.y);

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
        auto model = obj->GetComponent<BasicModel>();
        if (model && model->getBoundingBox().test(frustum))
        {
            out.push_back(obj);
        }
    }

    if (isLeaf()) return;

    for (const auto& child : m_children)
    {
        child->gatherObjects(frustum, out);
    }
}

void QuadNode::gatherRectangles(std::vector<BoundingRect>& out) const
{
    out.push_back(m_bounds);

    for (const auto& child : m_children)
    {
        if (child)
        {
            child->gatherRectangles(out);
        }
    }
}

