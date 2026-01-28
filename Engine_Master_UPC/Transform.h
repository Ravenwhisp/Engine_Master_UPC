#pragma once
#include "Component.h"

class Transform final : public Component{
public:
	Vector3& getPosition() { return position; }
private:
	Vector3 position;
};