#include "Globals.h"
#include "QuadNode.h"
#include "Quadtree.h"
#include "BasicModel.h"
#include "GameObject.h"

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
    if (!model)
    {
        return;
    }

    if (!m_bounds.contains(model->getAABB()))
    {
        return;
    }

    if (isLeaf())
    {
        if (m_objects.size() >= Quadtree::MAX_OBJECTS && m_depth < Quadtree::MAX_DEPTH)
        {
            subdivide();
            insertToChildren(object);
        }
        else
        {
            m_objects.push_back(&object);
            m_tree.m_objectLocationMap[&object] = this;
        }
    }
    else
    {
        insertToChildren(object);
    }
}

void QuadNode::remove(GameObject& object)
{
    auto& objs = m_objects;
    objs.erase(std::remove(objs.begin(), objs.end(), &object), objs.end());
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

    for (GameObject* obj : m_objects)
    {
        insertToChildren(*obj);
    }

    m_objects.clear();
}

void QuadNode::insertToChildren(GameObject& object)
{
    auto model = object.GetComponent<BasicModel>();
    if (!model)
    {
        return;
    }

    for (auto& child : m_children)
    {
        if (child->m_bounds.contains(model->getAABB()))
        {
            child->insert(object);
            return;
        }
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

bool QuadNode::intersects(const Frustum& frustum, const BoundingRect& rectangle, int minY, int maxY) const
{
    Plane* planes = frustum.getPlanes();

    Vector3 center;
    center.x = rectangle.x + rectangle.width * 0.5f;
    center.z = rectangle.y + rectangle.height * 0.5f;
    center.y = (minY + maxY) * 0.5f;

    Vector3 extents;
    extents.x = rectangle.width * 0.5f;
    extents.z = rectangle.height * 0.5f;
    extents.y = (maxY - minY) * 0.5f;

    for (int i = 0; i < Frustum::NUM_PLANES; ++i)
    {
        const Vector3 normal = planes[i].Normal();
        float distance = planes[i].DotCoordinate(center);

        float radius =
            fabsf(normal.x) * extents.x +
            fabsf(normal.y) * extents.y +
            fabsf(normal.z) * extents.z;

        if (distance - radius > 0.0f)
        {
            return false;
        }
    }
    return true;
}

void QuadNode::gatherObjects(const Frustum& frustum, std::vector<GameObject*>& out) const
{
    if (!intersects(frustum, m_bounds)) 
    { 
        return;
    }

    if (isLeaf())
    {
        for (GameObject* obj : m_objects)
        {
            auto model = obj->GetComponent<BasicModel>();
            if (!model)
            {
                return;
            }

            if (model->getAABB().test(frustum))
            {
                out.push_back(obj);
            }
        }

        return;
    }

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

