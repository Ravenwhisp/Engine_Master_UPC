#pragma once
#include "GameObject.h"
#include "QuadNode.h"

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

private:
	friend class QuadNode;

	std::unique_ptr<QuadNode> m_root;
	std::unordered_map<GameObject*, QuadNode*> m_objectLocationMap;
};