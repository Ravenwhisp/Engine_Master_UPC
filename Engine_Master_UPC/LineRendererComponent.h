#pragma once
#include "Component.h"
#include "AssetReference.h"
#include "imgui_color_gradient.h"
#include <vector>

class GameObject;


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

	AssetId& getTextureAssetReference() { return m_textureAsset; }

	std::vector<std::shared_ptr<RenderPoint>>& getPoints() { return m_points; }

	ImGradient& getColorGradient() { return m_color; }

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void serialize(IArchive& archive) override;

	void fixReferences(const SceneReferenceResolver& resolver) override;

	void debugDraw() override;

private:

	std::vector<std::shared_ptr<RenderPoint>> m_points;

	AssetId m_textureAsset{};

	ImGradient m_color;
	ImGradientMark* m_draggingMark = nullptr;
	ImGradientMark* m_selectedMark = nullptr;

	int m_selectedPoint = -1;

};

