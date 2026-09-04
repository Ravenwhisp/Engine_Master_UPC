#pragma once
#include "Component.h"
#include "imgui_color_gradient.h"
#include <vector>
#include "AssetReference.h"

class TrailComponent : public Component
{
public:

	struct TrailPoint
	{
		Vector3 position;
		Quaternion rotation;
		float lifeTime;
		float width;
		Vector4 color;
	};

	TrailComponent(UID id, GameObject* owner);

	void drawUi() override;

	void update() override;

	void CreatePoint();

	AssetId& getTextureAssetId() { return m_textureAsset; }

	std::vector<std::shared_ptr<TrailPoint>>& getTrailPoints() { return m_points; }

	ImGradient& getColorGradient() { return m_colorOverTime; }

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void serialize(IArchive& archive) override;

	void debugDraw() override;

	bool isGenerating() { return m_generate; }
	void generate(bool value) { m_generate = value; }

private:

	std::vector<std::shared_ptr<TrailPoint>> m_points;
	
	//Editable parameters
	float	m_startWidth;
	float	m_endWidth;
	float	m_spawnDistance;
	float	m_pointLifetime;
	
	AssetId m_textureAsset{};

	ImGradient m_colorOverTime;
	ImGradientMark* m_draggingMark = nullptr;
	ImGradientMark* m_selectedMark = nullptr;

	bool drawBezierCurveUI(float* curveData);
	float m_colorCurve[4] = { 0.000f, 0.000f, 1.000f, 1.000f };

	bool m_generate = false;
};


