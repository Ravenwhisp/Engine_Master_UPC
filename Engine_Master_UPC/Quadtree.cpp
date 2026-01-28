#include "Quadtree.h"

Quadtree::Quadtree(const DirectX::SimpleMath::Rectangle& area, UINT depth): bounds(area), depth(depth)
{
}

void Quadtree::insert(GameObject& gameObject)
{
	if (isLeaf()) 
	{
		if (objects.size() >= MAX_OBJECTS) 
		{
			subdivide();
			insertToChildren(gameObject);
		}
		else 
		{
			objects.push_back(&gameObject);
		}
	} 
	else 
	{
		insertToChildren(gameObject);
	}
}


void Quadtree::subdivide()
{
	if (depth >= MAX_DEPTH) return;

	float halfWidth = bounds.width / 2.0f;
	float halfHeight = bounds.height / 2.0f;

	children[0] = new Quadtree(DirectX::SimpleMath::Rectangle(bounds.x, bounds.y, halfWidth, halfHeight), depth + 1); // Top-left
	children[1] = new Quadtree(DirectX::SimpleMath::Rectangle(bounds.x + halfWidth, bounds.y, halfWidth, halfHeight), depth + 1); // Top-right
	children[2] = new Quadtree(DirectX::SimpleMath::Rectangle(bounds.x, bounds.y + halfHeight, halfWidth, halfHeight), depth + 1); // Bottom-left
	children[3] = new Quadtree(DirectX::SimpleMath::Rectangle(bounds.x + halfWidth, bounds.y + halfHeight, halfWidth, halfHeight), depth + 1); // Bottom-right

	for (auto& object : objects) {
		insert(*object);
	}
	objects.clear();
}


bool Quadtree::isInside(Quadtree& child, Vector3& position)
{
	if (child.bounds.Contains(position.x, position.y)) 
	{
		return true;
	}

	return false;
}

void Quadtree::insertToChildren(GameObject& gameObject)
{
	auto transform = gameObject.getTransform();

	for (auto& child : children)
	{
		if (isInside(*child, transform.getPosition()))
		{
			child->insert(gameObject);
			return;
		}
	}
}

std::vector<GameObject*> Quadtree::GetObjects(BoundingFrustum& frustum) const
{
	return std::vector<GameObject*>();
}
