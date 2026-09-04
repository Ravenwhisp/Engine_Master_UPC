#pragma once
#include "Component.h"
#include "AssetReference.h"
#include "imgui_color_gradient.h"
#include <vector>
#include <TextureAsset.h>

class GameObject;
class Texture;

class LineRendererComponent : public Component
{
public:

	struct RenderPoint
	{
		Vector3 position;
		float width;
		Transform* transformParent;
		uint64_t transformId;

	};

	LineRendererComponent(UID id, GameObject* owner);

	void drawUi() override;

	void update() override;

	void CreatePoint();
	float WrapAngle(float angle);

	void requestLoad() { m_loadRequested = true; }

	Texture* getTexture()			           const { return m_texture.get(); }
	const AssetId& getTextureAssetId()         const { return m_textureAssetId; }
	TextureAsset* getTextureAsset()            const { return m_textureAsset.get(); }

	bool consumeLoadRequest();
	void setTextureAssetId(const AssetId& assetId);

	std::vector<std::shared_ptr<RenderPoint>>& getPoints() { return m_points; }

	ImGradient& getColorGradient() { return m_color; }

	bool getBloomValue() { return m_bloom; }

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void serialize(IArchive& archive) override;

	void fixReferences(const SceneReferenceResolver& resolver) override;

	void debugDraw() override;

private:

	std::vector<std::shared_ptr<RenderPoint>> m_points;

	AssetId m_textureAssetId{};
	std::shared_ptr<Texture> m_texture = nullptr;
	std::shared_ptr<TextureAsset> m_textureAsset = nullptr;
	bool m_loadRequested = false;

	void LoadTexture(UID* data);

	ImGradient m_color;
	ImGradientMark* m_draggingMark = nullptr;
	ImGradientMark* m_selectedMark = nullptr;

	bool m_bloom = false;

	int m_selectedPoint = -1;

};

