#include "Globals.h"
#include "PlayToolbar.h"

#include "imgui.h"

#include "Application.h"
#include "EditorModule.h"

constexpr const char* PLAY_TEXT = "Play";
constexpr const char* PAUSE_TEXT = "Pause";
constexpr const char* STOP_TEXT = "Stop";

constexpr const char* BAR_FUNCTIONS[3] = { PLAY_TEXT, PAUSE_TEXT, STOP_TEXT };

enum SIMULATION_MODE
{
	PLAY = 0,
	PAUSE = 1,
	STOP = 2
};

PlayToolbar::PlayToolbar()
{
	m_moduleEditor = app->getEditorModule();
}

PlayToolbar::~PlayToolbar()
{
}

void PlayToolbar::DrawCentered(float menuWidth)
{
	float startX = ImGui::GetCursorPosX();
	float toolbarWidth = 3 * m_buttonWidth;
	float centerPos = (menuWidth - toolbarWidth) * 0.5f;
	ImGui::SetCursorPosX(centerPos);
	int selectedIndex = static_cast<int>(m_moduleEditor->getCurrentSimulationMode());
	for (int i = 0; i < 3; i++)
	{
		CreateButton(selectedIndex, BAR_FUNCTIONS[i], i);
	}
}

void PlayToolbar::ManagePositionButton(int selectedIndex)
{
	for (int i = 0; i < 3; i++)
	{
		CreateButton(selectedIndex, BAR_FUNCTIONS[i], i);
	}
}


void PlayToolbar::CreateButton(int selectedIndex, const char* text, int index)
{
	if (selectedIndex == index) 
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.55f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.65f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.45f, 1.0f, 1.0f));
    }
	else if (index == SIMULATION_MODE::PAUSE && selectedIndex == SIMULATION_MODE::STOP)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
	}
    if (ImGui::Button(text, ImVec2(m_buttonWidth, m_buttonHeight))) 
    {
		if (index == SIMULATION_MODE::PLAY)
		{ 
			if (selectedIndex == SIMULATION_MODE::PAUSE || selectedIndex == SIMULATION_MODE::STOP)
			{
				m_moduleEditor->setCurrentSimulationMode(index);
				app->setEngineState(index);
			}
		}
		if (index == SIMULATION_MODE::PAUSE)
		{
			if (selectedIndex == SIMULATION_MODE::PLAY)
			{
				m_moduleEditor->setCurrentSimulationMode(index);
				app->setEngineState(index);
			}
		}
		if (index == SIMULATION_MODE::STOP)
		{
			if (selectedIndex == SIMULATION_MODE::PLAY || selectedIndex == SIMULATION_MODE::PAUSE)
			{
				m_moduleEditor->setCurrentSimulationMode(index);
				app->setEngineState(index);
			}
		}
    }
    if (selectedIndex == index || (index == SIMULATION_MODE::PAUSE && selectedIndex == SIMULATION_MODE::STOP))
    {
        ImGui::PopStyleColor(3);
    }

    ImGui::SameLine();
}

