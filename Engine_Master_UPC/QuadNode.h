#pragma once
#include "GameObject.h"
#include <array>

class Quadtree;

struct RectangleData {
	float x, y, width, height;

	RectangleData() : x(0), y(0), width(0), height(0) {}
	RectangleData(float _x, float _y, float _width, float _height) : x(_x), y(_y), width(_width), height(_height) {}

	bool contains(const Vector3& point) const {
		return point.x >= x && point.x <= x + width && point.z >= y && point.z <= y + height;
	}
};

enum Quadrant {
	TOP_LEFT = 0,
	TOP_RIGHT = 1,
	BOTTOM_LEFT = 2,
	BOTTOM_RIGHT = 3,

	QUADRANT_COUNT = 4,
};

enum FrustrumPlane {
	NEAR_PLANE = 0,
	FAR_PLANE = 1,
	RIGHT_PLANE = 2,
	LEFT_PLANE = 3,
	TOP_PLANE = 4,
	BOTTOM_PLANE = 5,

	PLANE_COUNT = 6,
};


class QuadNode {
public:
	QuadNode(const RectangleData& bounds,
		UINT depth,
		Quadtree& tree,
		QuadNode* parent = nullptr);

	void insert(GameObject& object);
	void remove(GameObject& object);

	void gatherObjects(BoundingFrustum& frustum, std::vector<GameObject*>& out) const;

	void gatherRectangles(std::vector<RectangleData>& out) const;

	bool isLeaf() const { return m_children[TOP_LEFT] == nullptr; }

private:
	void subdivide();
	void insertToChildren(GameObject& object);
	void tryMergeUpwards();
	bool canMerge() const;
	void merge();

	bool intersects(const BoundingFrustum& frustum,
		const RectangleData& rectangle,
		int minY = -10000,
		int maxY = 10000) const;

private:
	RectangleData				m_bounds;
	int							m_depth;
	Quadtree& m_tree;
	QuadNode* m_parent = nullptr;
	std::array<std::unique_ptr<QuadNode>, QUADRANT_COUNT> m_children;
	std::vector<GameObject*>	m_objects;
};
