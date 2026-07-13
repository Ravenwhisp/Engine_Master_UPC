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
		Quaternion rotation;
		float width;
		Transform* transformParent;

	};

	LineRendererComponent(UID id, GameObject* owner);

	void drawUi() override;

	void update() override;

	void CreatePoint();

	AssetReference& getTextureAssetReference() { return m_textureAsset; }

	std::vector<std::shared_ptr<RenderPoint>>& getTrailPoints() { return m_points; }

	ImGradient& getColorGradient() { return m_color; }

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void serialize(IArchive& archive) override;

	void debugDraw() override;

private:

	std::vector<std::shared_ptr<RenderPoint>> m_points;

	AssetReference m_textureAsset{};

	ImGradient m_color;
	ImGradientMark* m_draggingMark = nullptr;
	ImGradientMark* m_selectedMark = nullptr;

};

