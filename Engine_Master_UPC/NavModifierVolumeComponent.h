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

	void drawUi() override;
	void onTransformChange() override;

	void serialize(IArchive& archive) override;
	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentInfo) override;

	void debugDraw() override;

	const Vector3& getHalfExtents() const { return m_halfExtents; }
	NavAreaType getAreaType() const { return m_areaType; }
	bool getEnabled() const { return m_enabled; }
	int getPriority() const { return m_priority; }

private:
	Vector3 m_halfExtents = Vector3::One; // Box half-size used during navmesh bake
	NavAreaType m_areaType = NavAreaType::Default; // Type of walkable area
	bool m_enabled = true; // True if it should affect navigation
	int m_priority = 0; // Higher priority overrides lower priority volumes
};