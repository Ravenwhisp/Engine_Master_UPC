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

		ImGui::ColorEdit4("Outline Color", &settings.outlineColor.x);

		float thickness = settings.outlineThickness;
		if (ImGui::DragFloat("Thickness", &thickness, 0.1f, 0.5f, 10.0f, "%.1f"))
		{
			settings.outlineThickness = thickness;
		}
	}
}
