#pragma once
#include <vector>
#include "Component.h"
#include "SimpleMath.h"

class GameObject;

class Transform final : public Component {
public:
	Transform(int id, GameObject* gameObject);

	const Matrix& getGlobalMatrix() const;
	const Vector3* getPosition() { return &m_position; }
	const Quaternion* getRotation() { return &m_rotation; }
	const Vector3* getScale() { return &m_scale; }

	void setPosition(const Vector3 &newPosition) { m_position = newPosition; markDirty(); }
	void setRotation(const Quaternion& newRotation) { m_rotation = newRotation; markDirty(); }
	void setRotation(const Vector3 &newRotation)
	{
		m_rotation = Quaternion::CreateFromYawPitchRoll(XMConvertToRadians(newRotation.y), XMConvertToRadians(newRotation.x), XMConvertToRadians(newRotation.z));
		markDirty();
	}
	void setScale(const Vector3 &newScale) { m_scale = newScale;  markDirty(); }
	void markDirty();

#pragma region Hierarchy Scene
	Transform* getRoot() const { return m_root; }
	const std::vector<GameObject*>& getAllChildren() const { return m_children; }

	void setRoot(Transform* root) { m_root = root; }
	void addChild(GameObject* child) { m_children.push_back(child); }
	void removeChild(int id);

	bool isDescendantOf(const Transform* potentialParent) const;
#pragma endregion

	void drawUi() override;

private:
	mutable Matrix m_globalMatrix;
	mutable bool m_dirty;

	Vector3 m_position;
	Quaternion m_rotation;
	Vector3 m_scale;

	Transform* m_root;
	std::vector<GameObject*> m_children;

	void calculateMatrix() const;
	Vector3 convertQuaternionToEulerAngles(const Quaternion &rotation);
};