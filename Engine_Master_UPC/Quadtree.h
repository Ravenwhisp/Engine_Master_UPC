#pragma once
#include <Frustum.h>
#include "QuadNode.h"

class GameObject;

class Quadtree 
{
public:
	static const int MAX_OBJECTS = 1;
	static const int MAX_DEPTH = 5;

	Quadtree(const BoundingRect& worldBounds);

	void insert(GameObject& object);
	void remove(GameObject& object);
	void move(GameObject& object);

	std::vector<GameObject*> getObjects(const Engine::Frustum& frustum) const;
	std::vector<BoundingRect> getQuadrants() const;
	QuadNode& getRoot() { return *m_root; }
private:
	friend class QuadNode;

	std::unique_ptr<QuadNode> m_root;
	std::unordered_map<GameObject*, QuadNode*> m_objectLocationMap;
};