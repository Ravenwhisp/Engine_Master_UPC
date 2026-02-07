#include "Quadtree.h"


Quadtree::Quadtree(const DirectX::SimpleMath::Rectangle& area, UINT depth): m_bounds(area), m_depth(depth)
{

}

void Quadtree::insert(GameObject& gameObject)
{
	if (isLeaf()) 
	{
		if (m_objects.size() >= MAX_OBJECTS) 
		{
			subdivide();
			insertToChildren(gameObject);
		}
		else 
		{
			m_objectLocationMap[&gameObject] = this;
			m_objects.push_back(&gameObject);
		}
	} 
	else 
	{
		insertToChildren(gameObject);
	}
}

void Quadtree::move(GameObject& movedObjects)
{
	remove(movedObjects);
	insert(movedObjects);
}

void Quadtree::remove(GameObject& gameObject)
{
	Quadtree* location = m_objectLocationMap[&gameObject];
	if (location) 
	{
		auto& objs = location->m_objects;
		objs.erase(std::remove(objs.begin(), objs.end(), &gameObject), objs.end());
		m_objectLocationMap.erase(&gameObject);
	}
}


void Quadtree::subdivide()
{
	if (m_depth >= MAX_DEPTH) return;

	float halfWidth = m_bounds.width / 2.0f;
	float halfHeight = m_bounds.height / 2.0f;

	m_children[0] = new Quadtree(DirectX::SimpleMath::Rectangle(m_bounds.x, m_bounds.y, halfWidth, halfHeight), m_depth + 1); // Top-left
	m_children[1] = new Quadtree(DirectX::SimpleMath::Rectangle(m_bounds.x + halfWidth, m_bounds.y, halfWidth, halfHeight), m_depth + 1); // Top-right
	m_children[2] = new Quadtree(DirectX::SimpleMath::Rectangle(m_bounds.x, m_bounds.y + halfHeight, halfWidth, halfHeight), m_depth + 1); // Bottom-left
	m_children[3] = new Quadtree(DirectX::SimpleMath::Rectangle(m_bounds.x + halfWidth, m_bounds.y + halfHeight, halfWidth, halfHeight), m_depth + 1); // Bottom-right

	for (auto& object : m_objects)
	{
		insert(*object);
	}

	m_objects.clear();
}


bool Quadtree::isInside(Quadtree& quad, Vector3& position)
{
	return quad.m_bounds.Contains(position.x, position.y);
}

void Quadtree::insertToChildren(GameObject& gameObject)
{
	auto transform = gameObject.getTransform();

	for (Quadtree* child : m_children)
	{
		if (isInside(*child, transform.getPosition()))
		{
			child->insert(gameObject);
			return;
		}
	}
}


Plane* Quadtree::getFrustumPlanes(const BoundingFrustum& frustum) const
{
	XMVECTOR planes[PLANE_COUNT];
	frustum.GetPlanes(&planes[NEAR_PLANE], &planes[FAR_PLANE], &planes[RIGHT_PLANE], &planes[LEFT_PLANE], &planes[TOP_PLANE], &planes[BOTTOM_PLANE]);

	static Plane result[PLANE_COUNT] =
	{
		Plane(planes[NEAR_PLANE]),
		Plane(planes[FAR_PLANE]),
		Plane(planes[RIGHT_PLANE]),
		Plane(planes[LEFT_PLANE]),
		Plane(planes[TOP_PLANE]),
		Plane(planes[BOTTOM_PLANE])
	};

	return result;
}

bool Quadtree::intersects(const Plane* planes, const Vector3& center, const Vector3& extents) const 
{
	Vector3 axisX = Vector3::UnitX;
	Vector3 axisY = Vector3::UnitY;
	Vector3 axisZ = Vector3::UnitZ;

	for (int i = 0; i < PLANE_COUNT; i++)
	{
		Vector3 normal = planes[i].Normal();
		float distance = planes[i].DotCoordinate(center);

		float radius =
			fabsf(normal.Dot(axisX)) * extents.x +
			fabsf(normal.Dot(axisY)) * extents.y +
			fabsf(normal.Dot(axisZ)) * extents.z;

		if (distance + radius < 0.0f)
		{
			return false;
		}
	}

	return true;
}

bool Quadtree::intersects(const BoundingFrustum& frustum, const BoundingBox& box) const
{
	Plane* planes = getFrustumPlanes(frustum);
	const Vector3& center = box.Center;
	const Vector3& extents = box.Extents;
	return intersects(planes, center, extents);
}

bool Quadtree::intersects(const BoundingFrustum& frustum, const DirectX::SimpleMath::Rectangle& rectangle, int minY = -1000, int maxY = 1000) const
{
	Plane* planes = getFrustumPlanes(frustum);

	Vector3 center;
	center.x = rectangle.x + rectangle.width * 0.5f;
	center.z = rectangle.y + rectangle.height * 0.5f;
	center.y = (minY + maxY) * 0.5f;

	Vector3 extents;
	extents.x = rectangle.width * 0.5f;
	extents.z = rectangle.height * 0.5f;
	extents.y = (maxY - minY) * 0.5f;

	return intersects(planes, center, extents);
}


std::vector<GameObject*> Quadtree::getObjects(BoundingFrustum& frustum) const
{
	std::vector<GameObject*> result;
	gatherObjects(frustum, result);
	return result;
}

void Quadtree::gatherObjects(BoundingFrustum& frustum, std::vector<GameObject*>& outObjects) const
{
	if (!intersects(frustum, m_bounds)) 
	{
		return;
	}

	if (isLeaf())
	{
		for (GameObject* obj : m_objects)
		{
			//PLACEHOLDER
			BoundingBox box = BoundingBox();
			if (intersects(frustum, box))
				outObjects.push_back(obj);
		}
		return;
	}

	for (const Quadtree* child : m_children)
	{
		if (child) 
		{
			child->gatherObjects(frustum, outObjects);
		}
	}
}

