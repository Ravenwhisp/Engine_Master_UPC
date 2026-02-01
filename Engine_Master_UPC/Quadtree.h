#pragma once
#include "SimpleMath.h"
#include "GameObject.h"

using namespace DirectX::SimpleMath;
using namespace DirectX;

enum FrustrumPlane {
	NEAR_PLANE = 0,
	FAR_PLANE = 1,
	RIGHT_PLANE = 2,
	LEFT_PLANE = 3,
	TOP_PLANE = 4,
	BOTTOM_PLANE = 5,

	PLANE_COUNT = 6,
};


enum Quadrant {
	TOP_LEFT = 0,
	TOP_RIGHT = 1,
	BOTTOM_LEFT = 2,
	BOTTOM_RIGHT = 3,

	QUADRANT_COUNT = 4,
};

class Quadtree {
public:
	static const int MAX_OBJECTS = 1;
	static const int MAX_DEPTH = 5;

	Quadtree(const DirectX::SimpleMath::Rectangle& area, UINT depth = 0);
	void insert(GameObject& gameObject);
	void move(GameObject& movedObjects);
	void remove(GameObject& gameObject);

	std::vector<GameObject*> getObjects(BoundingFrustum& frustum) const;
private:
	void subdivide();
	bool isLeaf() const { return m_children[TOP_LEFT] == nullptr; }
	bool isInside(Quadtree& quad, Vector3& position);
	void insertToChildren(GameObject& gameObject);

	Plane* getFrustumPlanes(const BoundingFrustum& frustum) const;

	bool intersects(const Plane* planes, const Vector3& center, const Vector3& extents) const;
	bool intersects(const BoundingFrustum& frustum, const BoundingBox& box) const;
	bool intersects(const BoundingFrustum& frustum, const DirectX::SimpleMath::Rectangle& rectangle, int minY = -1000, int maxY = 1000) const;

	void gatherObjects(BoundingFrustum& frustum,  std::vector<GameObject*>& outObjects) const;

	DirectX::SimpleMath::Rectangle m_bounds;
	std::vector<GameObject*> m_objects;
	UINT m_depth = 0;

	Quadtree* m_children[QUADRANT_COUNT] = { nullptr, nullptr, nullptr, nullptr };

	std::unordered_map<GameObject*, Quadtree*> m_objectLocationMap;
};