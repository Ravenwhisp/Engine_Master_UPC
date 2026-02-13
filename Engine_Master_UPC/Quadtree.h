#pragma once
#include "GameObject.h"
#include "QuadNode.h"
#include <Frustum.h>


class Quadtree {
public:
	static const int MAX_OBJECTS = 1;
	static const int MAX_DEPTH = 5;

	Quadtree(const BoundingRect& worldBounds);

	void insert(GameObject& object);
	void remove(GameObject& object);
	void move(GameObject& object);

	std::vector<GameObject*> getObjects(const Frustum& frustum) const;
	std::vector<BoundingRect> getQuadrants() const;
private:
	friend class QuadNode;

	std::unique_ptr<QuadNode> m_root;
	std::unordered_map<GameObject*, QuadNode*> m_objectLocationMap;
};