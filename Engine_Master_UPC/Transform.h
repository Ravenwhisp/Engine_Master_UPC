#pragma once
#include "Component.h"

#include <vector>
#include "SimpleMath.h"

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Quaternion;

class GameObject;

class Transform final : public Component 
{
public:
	Transform(UID id, GameObject* gameObject);
	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	const Matrix& getGlobalMatrix() const;
	Matrix getNormalMatrix() const;
	void setFromGlobalMatrix(const Matrix &worldMatrix);

	const Vector3& getPosition() const { return m_position; }
	const Quaternion& getRotation() const { return m_rotation; }
	const Vector3& getScale() const { return m_scale; }

	void setPosition(const Vector3& newPosition);
	void setRotation(const Quaternion& newRotation);
	void setRotationEuler(const Vector3& eulerDegrees);
	void setScale(const Vector3 &newScale) { m_scale = newScale;  markDirty(); }
	void markDirty();
	bool isDirty() { return m_dirty; }

	Vector3 getForward() const;
	Vector3 getRight() const;
	Vector3 getUp() const;

	const Vector3& getEulerDegrees() const { return m_eulerDegrees; }

	void onTransformChange() override {}

#pragma region WindowHierarchy Scene
	Transform* getRoot() const { return m_root; }
	const std::vector<GameObject*>& getAllChildren() const { return m_children; }

	void setRoot(Transform* root);
	void addChild(GameObject* child) { m_children.push_back(child); markDirty(); }
	void removeChild(UID id);

	bool isDescendantOf(const Transform* potentialParent) const;
#pragma endregion

	void drawUi() override;

#pragma region Filesystem
	rapidjson::Value getJSON(rapidjson::Document& domTree) override; // only the basics! (no children nor parent)
#pragma endregion

private:
	mutable Matrix m_globalMatrix;
	mutable bool m_dirty;

	Vector3 m_position;
	Quaternion m_rotation;
	Vector3 m_eulerDegrees;
	Vector3 m_scale;

	Transform* m_root;
	std::vector<GameObject*> m_children;

	void calculateMatrix() const;
};