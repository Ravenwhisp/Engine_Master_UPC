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

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentInfo) override;

	void debugDraw() override;

	const Vector3& getHalfExtents() const { return m_halfExtents; }
	NavAreaType getAreaType() const { return m_areaType; }
	bool getEnabled() const { return m_enabled; }
	int getPriority() const { return m_priority; }

private:
	Vector3 m_halfExtents = Vector3::One;
	NavAreaType m_areaType = NavAreaType::Default;
	bool m_enabled = true;
	int m_priority = 0;
};