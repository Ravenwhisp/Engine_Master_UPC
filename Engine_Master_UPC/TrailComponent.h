#pragma once
#include "Component.h"
#include <vector>

class TrailComponent : public Component
{
	struct TrailPoint
	{
		Vector3 position;
		Quaternion rotation;
		float lifeTime = 0.f;
	};

public:

	TrailComponent(UID id, GameObject* owner);
	void drawUi() override;

	std::vector<TrailPoint*>& getTrailPoints() { return m_points; }

	ImGradient& getColorGradient() { return m_colorOverTime; }


private:

	std::vector<TrailPoint*> m_points;
	
	//Editable parameters
	float	m_startWidth;
	float	m_endWidth;
	float	m_spawnDistance;
	float	m_pointLifetime;
	

	ImGradient m_colorOverTime;
	ImGradientMark* m_draggingMark = nullptr;
	ImGradientMark* m_selectedMark = nullptr;
};

