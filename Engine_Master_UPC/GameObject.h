#pragma once
#include "Transform.h"
#include "Component.h"

class GameObject {
public:
	GameObject();

	Transform& getTransform() { return transform; }
	const Transform& getTransform() const { return transform; }

private:
	Transform transform;
};