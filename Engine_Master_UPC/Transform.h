#pragma once
#include <vector>
#include "Component.h"

class GameObject;

class Transform final : public Component {
public:
	const Vector3* getPosition() { return &m_position; }
	const Quaternion* getRotation() { return &m_rotation; }
	const Vector3* getScale() { return &m_scale; }
	const GameObject* getParent() { return m_parent; }
	const Transform* getRoot() { return m_root; }
	const std::vector<GameObject*>* getAllChildren() { return &m_children; }
	const Matrix* getTransformation();

	const void setPosition(Vector3* newPosition) { m_position = *newPosition; m_dirty = true; }
	const void setRotation(Quaternion* newRotation);
	const void setRotation(Vector3* newRotation);
	const void setScale(Vector3* newScale) { m_scale = *newScale;  m_dirty = true; }

	const void setParent(GameObject* parent) { m_parent = parent; }
	const void setRoot(Transform* root) { m_root = root; }
	const void addChild(GameObject* child) { m_children.push_back(child); }

	const void translate(Vector3* position);
	const void rotate(Vector3* eulerAngles);
	const void rotate(Quaternion* rotation);
	const void scalate(Vector3* scale);

	const Vector3 convertQuaternionToEulerAngles(const Quaternion* rotation);

	void drawUi() override;

private:
	Vector3 m_position;
	Quaternion m_rotation;
	Vector3 m_scale;
	Matrix m_transformation;
	bool m_dirty;

	GameObject* m_parent;
	Transform* m_gameObject;
	Transform* m_root;
	std::vector<GameObject*> m_children;

	const void calculateMatrix();

};