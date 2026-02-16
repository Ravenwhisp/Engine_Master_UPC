#pragma once
#include <array>
#include "BoundingBox.h"

class Quadtree;
class GameObject;

struct BoundingRect 
{
	float x, y, width, height;

	// DEBUG
	mutable bool m_debugIsCulled = true;

	BoundingRect() : x(0), y(0), width(0), height(0) {}
	BoundingRect(float _x, float _y, float _width, float _height) : x(_x), y(_y), width(_width), height(_height) {}

	float minX() const { return x; }
	float maxX() const { return x + width; }
	float minZ() const { return y; }
	float maxZ() const { return y + height; }

	bool contains(const Vector3& point) const 
	{
		return point.x >= x && point.x <= x + width && point.z >= y && point.z <= y + height;
	}

	bool contains(const Engine::BoundingBox& box) const
	{
		Vector3 minPointInWorldSpace = box.getMinInWorldSpace();
		Vector3 maxPointInWorldSpace = box.getMaxInWorldSpace();
		
		return minPointInWorldSpace.x >= minX() ||
			maxPointInWorldSpace.x <= maxX() ||
			minPointInWorldSpace.z >= minZ() ||
			maxPointInWorldSpace.z <= maxZ();
	}

	bool intersects(const Engine::BoundingBox& box) const
	{
		Vector3 minPointInWorldSpace = box.getMinInWorldSpace();
		Vector3 maxPointInWorldSpace = box.getMaxInWorldSpace();

		// Project the 3D AABB to XZ
		float boxMinX = minPointInWorldSpace.x;
		float boxMaxX = maxPointInWorldSpace.x;
		float boxMinZ = minPointInWorldSpace.z;
		float boxMaxZ = maxPointInWorldSpace.z;

		// Separating Axis Theorem (2D AABB vs AABB)
		if (boxMaxX < minX()) return false;
		if (boxMinX > maxX()) return false;
		if (boxMaxZ < minZ()) return false;
		if (boxMinZ > maxZ()) return false;

		return true;
	}
};

enum Quadrant 
{
	TOP_LEFT = 0,
	TOP_RIGHT = 1,
	BOTTOM_LEFT = 2,
	BOTTOM_RIGHT = 3,

	QUADRANT_COUNT = 4,
};

class QuadNode 
{
public:
	QuadNode(const BoundingRect& bounds,
		UINT depth,
		Quadtree& tree,
		QuadNode* parent = nullptr);

	void insert(GameObject& object);
	void remove(GameObject& object);

	void gatherObjects(const Engine::Frustum& frustum, std::vector<GameObject*>& out) const;

	void gatherRectangles(std::vector<BoundingRect>& out) const;

	bool isLeaf() const { return m_children[TOP_LEFT] == nullptr; }

	BoundingRect getBounds() const { return m_bounds; }

private:
	void subdivide();
	void insertToChildren(GameObject& object);
	void tryMergeUpwards();
	bool canMerge() const;
	void merge();

	bool intersects(const Engine::Frustum& frustum,
		const BoundingRect& rectangle,
		int minY = -10000,
		int maxY = 10000) const;

private:
	BoundingRect				m_bounds;
	int							m_depth;
	Quadtree&					m_tree;
	QuadNode*					m_parent = nullptr;
	std::array<std::unique_ptr<QuadNode>, QUADRANT_COUNT> m_children;
	std::vector<GameObject*>	m_objects;
};
