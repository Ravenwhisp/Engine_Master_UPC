#include "Globals.h"
#include "WindowOutlineSettings.h"
#include "Application.h"
#include "ModuleScene.h"
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

		ImGui::ColorEdit4("Color", &settings.outlineColor.x);

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
		ImGui::DragFloat("Noise", &settings.noiseScale, 0.5f, 0.0f, 20.0f, "%.1f");
	}
}
