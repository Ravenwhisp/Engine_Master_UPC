#pragma once

#include "Component.h"
#include "NavMeshTypes.h"
#include "SimpleMath.h"

using DirectX::SimpleMath::Vector3;

class NavModifierVolumeComponent final : public Component
{
public:
	NavModifierVolumeComponent(UID id, GameObject* owner);
	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

private:
	Vector3 halfExtents = Vector3::One;
	NavAreaType areaType = NavAreaType::Default;
	bool enabled = true;
	int priority = 0;
};