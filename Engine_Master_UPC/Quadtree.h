#pragma once
#include "GameObject.h"
#include "QuadNode.h"

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

	Quadtree(const RectangleData& worldBounds);

	void insert(GameObject& object);
	void remove(GameObject& object);
	void move(GameObject& object);

	std::vector<GameObject*> getObjects(BoundingFrustum& frustum) const;
	std::vector<RectangleData> getQuadrants() const;

	std::vector<GameObject*> getObjects(BoundingFrustum& frustum) const;
private:
	friend class QuadNode;

	std::unique_ptr<QuadNode> m_root;
	std::unordered_map<GameObject*, QuadNode*> m_objectLocationMap;
};