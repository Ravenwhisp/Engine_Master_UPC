#pragma once

#include "Component.h"
#include "SimpleMath.h"

using DirectX::SimpleMath::Vector3;

class NavRuntimeBlockerComponent final : public Component
{
public:
	NavRuntimeBlockerComponent(UID id, GameObject* owner);
	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void drawUi() override;
	void onTransformChange() override;

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentInfo) override;

	void debugDraw() override;

	const Vector3& getHalfExtents() const { return m_halfExtents; }
	
	void setBlocked(bool blocked);
	bool isBlocked() const { return m_blocked; }

private:
	Vector3 m_halfExtents = Vector3::One; // Box half-size
	bool m_blocked = false; // if navmesh is blocked
};