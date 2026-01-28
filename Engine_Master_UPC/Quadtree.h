#pragma once
#include "SimpleMath.h"
#include "GameObject.h"

using namespace DirectX::SimpleMath;
using namespace DirectX;

enum Quadrant {
	TOP_LEFT = 0,
	TOP_RIGHT = 1,
	BOTTOM_LEFT = 2,
	BOTTOM_RIGHT = 3
};

class Quadtree {
public:
	static const int MAX_OBJECTS = 1;
	static const int MAX_DEPTH = 5;

	Quadtree(const DirectX::SimpleMath::Rectangle& area, UINT depth = 0);
	void insert(GameObject& gameObject);
private:
	void subdivide();
	bool isLeaf() const { return children[TOP_LEFT] == nullptr; }
	bool isInside(Quadtree& child, Vector3& position);
	void insertToChildren(GameObject& gameObject);

	std::vector<GameObject*> GetObjects(BoundingFrustum& frustum) const;

	DirectX::SimpleMath::Rectangle bounds;
	std::vector<GameObject*> objects;
	UINT depth = 0;

	Quadtree* children[4] = { nullptr, nullptr, nullptr, nullptr };
};