#pragma once
#include <vector>
#include "Component.h"

class Transform final : public Component{
public:
	const Vector3* getPosition() { return &m_position; }
	const Quaternion* getRotation() { return &m_rotation; }
	const Vector3* getEulerAngles() { return &m_eulerAngles; }
	const Vector3* getScale() { return &m_scale; }
	const Transform* getParent() { return m_parent; }
	const Transform* getChild(int index) { return m_children[index]; }
	const Matrix* getTransformation();


	const Transform* findChild(char* name);

	const void setPosition(Vector3* newPosition) { m_position = *newPosition; dirty = true; }
	const void setRotation(Quaternion* newRotation);
	const void setRotation(Vector3* newRotation);
	const void setScale(Vector3* newScale) { m_scale = *newScale;  dirty = true; }

	const void setParent(Transform* parent) { m_parent = parent; }
	const void addChild(Transform* child) { m_children.push_back(child); }
	const void removeChild(Transform* child);

	const void translate(Vector3* position);
	const void rotate(Vector3* eulerAngles);
	const void rotate(Quaternion* rotation);
	const void scalate(Vector3* scale);

	const Vector3 convertQuaternionToEulerAngles(Quaternion* rotation);

private:
	Vector3 m_position;
	Quaternion m_rotation;
	Vector3 m_eulerAngles;
	Vector3 m_scale;
	Matrix m_transformation;
	bool dirty;

	Transform* m_parent;
	Transform* root;
	std::vector<Transform*> m_children;

	const void calculateMatrix();
};