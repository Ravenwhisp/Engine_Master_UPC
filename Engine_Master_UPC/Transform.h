#pragma once
#include <vector>
#include "Component.h"
#include "SimpleMath.h"

class GameObject;

class Transform final : public Component {
public:
	Transform(int id, GameObject* gameObject) : Component(id, ComponentType::TRANSFORM, gameObject),
		m_dirty(false), m_root(nullptr), m_transformation(Matrix().Identity)
	{};

	const Matrix* getTransformation();
	const void setTransformation(const Matrix& newTransformation) { m_transformation = newTransformation; }

	const Vector3* getPosition() { return &m_position; }
	const Quaternion* getRotation() { return &m_rotation; }
	const Vector3* getScale() { return &m_scale; }

	void setPosition(const Vector3 &newPosition) { m_position = newPosition; m_dirty = true; }
	void setRotation(const Quaternion &newRotation);
	void setRotation(const Vector3 &newRotation);
	void setScale(const Vector3 &newScale) { m_scale = newScale;  m_dirty = true; }
	
	void translate(Vector3* position);
	void rotate(Vector3* eulerAngles);
	void rotate(Quaternion* rotation);
	void scalate(Vector3* scale);

	const Transform* getRoot() { return m_root; }
	const std::vector<GameObject*>* getAllChildren() { return &m_children; }

	void setRoot(Transform* root) { m_root = root; }
	void addChild(GameObject* child) { m_children.push_back(child); }

	const Vector3 convertQuaternionToEulerAngles(const Quaternion* rotation);

	void drawUi() override;

private:
	Matrix m_transformation;
	bool m_dirty;

	Vector3 m_position;
	Quaternion m_rotation;
	Vector3 m_scale;

	Transform* m_root;
	std::vector<GameObject*> m_children;

	const void calculateMatrix();
};