#pragma once
#include "Component.h"
#include "imgui_color_gradient.h"
#include <vector>

class TrailComponent : public Component
{
public:

	struct TrailPoint
	{
		Vector3 position;
		Quaternion rotation;
		float lifeTime;
		float width;

	};

	TrailComponent(UID id, GameObject* owner);

	void drawUi() override;

	void update() override;

	void CreatePoint();

	std::vector<std::shared_ptr<TrailPoint>>& getTrailPoints() { return m_points; }

	ImGradient& getColorGradient() { return m_colorOverTime; }

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentInfo) override;

	void debugDraw() override;


private:

	std::vector<std::shared_ptr<TrailPoint>> m_points;
	
	//Editable parameters
	float	m_startWidth;
	float	m_endWidth;
	float	m_spawnDistance;
	float	m_pointLifetime;
	

	ImGradient m_colorOverTime;
	ImGradientMark* m_draggingMark = nullptr;
	ImGradientMark* m_selectedMark = nullptr;

};


