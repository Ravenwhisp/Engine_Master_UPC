#pragma once
#include "Component.h"
#include <vector>

class TrailComponent : public Component
{
	struct TrailPoint
	{
		Vector3 position;
		Quaternion rotation;
		float lifeTime;
		float width;

	};

public:

	TrailComponent(UID id, GameObject* owner);

	void drawUi() override;

	void update() override;

	void CreatePoint();

	std::vector<std::shared_ptr<TrailPoint>>& getTrailPoints() { return m_points; }

	ImGradient& getColorGradient() { return m_colorOverTime; }


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


