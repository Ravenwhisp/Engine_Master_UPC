#include "Globals.h"
#include "WindowOutlineSettings.h"
#include "Application.h"
#include "ModuleScene.h"
#include "ModuleAssets.h"
#include "Scene.h"
#include "OutlineSettings.h"

WindowOutlineSettings::WindowOutlineSettings()
{
}

void WindowOutlineSettings::drawInternal()
{
	auto* scene = app->getModuleScene()->getScene();
	if (!scene)
	{
		ImGui::TextDisabled("No scene loaded.");
		return;
	}

	auto& settings = scene->getOutlineSettings();

	if (ImGui::CollapsingHeader("Outline Effect", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bool enabled = settings.enabled;
		if (ImGui::Checkbox("Enabled", &enabled))
		{
			settings.enabled = enabled;
		}

		ImGui::ColorEdit4("Color Modifier", &settings.colorModifier.x);

		ImGui::Separator();
		ImGui::Text("Radius");
		ImGui::DragFloat("Near##radius", &settings.maxSeparation, 0.5f, 0.5f, 15.0f, "%.1f");
		ImGui::DragFloat("Far##radius",  &settings.minSeparation, 0.5f, 0.0f, 10.0f, "%.1f");

		ImGui::Separator();
		ImGui::Text("Sensitivity");
		ImGui::DragFloat("Min##sens", &settings.minDistance, 0.001f, 0.0f, 0.2f, "%.3f");
		ImGui::DragFloat("Max##sens", &settings.maxDistance, 0.001f, 0.0f, 0.5f, "%.3f");

		ImGui::Separator();
		ImGui::Text("Search");
		ImGui::SliderInt("Window Size", &settings.searchSize, 1, 4);

		ImGui::Separator();
		ImGui::Text("Sketchy");
		ImGui::DragFloat("Noise Scale", &settings.noiseScale, 0.5f, 0.0f, 20.0f, "%.1f");

		const char* dropLabel = settings.noiseTextureAssetId.isValid()
			? "Drop Noise Texture (Assigned)"
			: "Drop Noise Texture Here";
		ImGui::Button(dropLabel);

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
			{
				AssetReference* data = static_cast<AssetReference*>(payload->Data);
				if (auto ref = app->getModuleAssets()->findReference(data->m_uid))
				{
					settings.noiseTextureAssetId = *ref;
				}
			}

			ImGui::EndDragDropTarget();
		}
	}
}
