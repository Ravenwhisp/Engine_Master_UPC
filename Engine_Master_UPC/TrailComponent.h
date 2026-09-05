#pragma once
#include "Component.h"
#include "imgui_color_gradient.h"
#include <vector>
#include "AssetReference.h"
#include <TextureAsset.h>

class Texture;

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


	void requestLoad() { m_loadRequested = true; }

	Texture* getTexture()			           const { return m_texture.get(); }
	const AssetId& getTextureAssetId()         const { return m_textureAssetId; }
	TextureAsset* getTextureAsset()            const { return m_textureAsset.get(); }

	bool consumeLoadRequest();
	void setTextureAssetId(const AssetId& assetId);


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
	
	AssetId m_textureAssetId{};
	std::shared_ptr<Texture> m_texture = nullptr;
	std::shared_ptr<TextureAsset> m_textureAsset = nullptr;
	bool m_loadRequested = false;

	void LoadTexture(UID* data);

	ImGradient m_colorOverTime;
	ImGradientMark* m_draggingMark = nullptr;
	ImGradientMark* m_selectedMark = nullptr;

	bool drawBezierCurveUI(float* curveData);
	float m_colorCurve[4] = { 0.000f, 0.000f, 1.000f, 1.000f };

	bool m_bloom = false;

	bool m_generate = true;
};


