#pragma once

#include <array>
#include <vector>
#include <memory>

#include "BoundingBox.h"

class Quadtree;
class GameObject;

struct BoundingRect
{
    float x, y, width, height;

    mutable bool m_debugIsCulled = true;

    BoundingRect() : x(0), y(0), width(0), height(0) {}
    BoundingRect(float _x, float _y, float _width, float _height)
        : x(_x), y(_y), width(_width), height(_height) {
    }

    float minX() const { return x; }
    float maxX() const { return x + width; }
    float minZ() const { return y; }
    float maxZ() const { return y + height; }

    bool containsFully(const Engine::BoundingBox& box) const
    {
        const Vector3 min = box.getMinInWorldSpace();
        const Vector3 max = box.getMaxInWorldSpace();

        return
            min.x >= minX() &&
            max.x <= maxX() &&
            min.z >= minZ() &&
            max.z <= maxZ();
    }

    bool intersects(const Engine::BoundingBox& box) const
    {
        const Vector3 min = box.getMinInWorldSpace();
        const Vector3 max = box.getMaxInWorldSpace();

        if (max.x < minX()) return false;
        if (min.x > maxX()) return false;
        if (max.z < minZ()) return false;
        if (min.z > maxZ()) return false;

        return true;
    }

    bool contains(const Vector3& point) const
    {
        return point.x >= x &&
            point.x <= x + width &&
            point.z >= y &&
            point.z <= y + height;
    }

    bool contains(const Engine::BoundingBox& box) const
    {
        Vector3 min = box.getMinInWorldSpace();
        Vector3 max = box.getMaxInWorldSpace();

        float qMinX = minX();
        float qMaxX = maxX();
        float qMinZ = minZ();
        float qMaxZ = maxZ();

        bool overlapX = !(max.x < qMinX || min.x > qMaxX);
        bool overlapZ = !(max.z < qMinZ || min.z > qMaxZ);

        return overlapX && overlapZ;
    }
};

enum Quadrant
{
    TOP_LEFT = 0,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    QUADRANT_COUNT
};

class QuadNode
{
public:

    QuadNode(
        const BoundingRect& bounds,
        UINT depth,
        Quadtree& tree,
        QuadNode* parent = nullptr);

    void insert(GameObject& object);
    void refit(GameObject& object);
    void remove(GameObject& object);

    void gatherObjects(const Engine::Frustum& frustum, std::vector<GameObject*>& out) const;
    void gatherRectangles(std::vector<BoundingRect>& out) const;

    int getDepth() const { return m_depth; }

    void clearDirty() { m_dirty = false; }

    bool canMerge() const;
    void merge();

private:

    void subdivide();
    void markDirty();

    bool isLeaf() const { return m_children[0] == nullptr; }

    bool intersects(const Engine::Frustum& frustum, const BoundingRect& rectangle) const;

    QuadNode* findBestFitChild(const Engine::BoundingBox& box) const;
    QuadNode* findContainingAncestor(const Engine::BoundingBox& box);

private:

    bool m_dirty = false;

    BoundingRect m_bounds;

    int m_depth = 0;

    Quadtree& m_tree;

    QuadNode* m_parent = nullptr;

    std::array<std::unique_ptr<QuadNode>, QUADRANT_COUNT> m_children;

    std::vector<GameObject*> m_objects;
};