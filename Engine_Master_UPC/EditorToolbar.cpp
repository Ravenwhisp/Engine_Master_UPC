#include "Globals.h"
#include "EditorToolbar.h"
#include "imgui.h"

#include "Application.h"
#include "EditorModule.h"

constexpr const char* PAN_TEXT = "Pan";
constexpr const char* ORBIT_TEXT = "Orbit";
constexpr const char* ZOOM_TEXT = "Zoom";
constexpr const char* FLY_MODE_TEXT = "Fly mode";

constexpr const char* VIEW_NAV_MODES[4] = { PAN_TEXT, ORBIT_TEXT, ZOOM_TEXT, FLY_MODE_TEXT };

constexpr const char* MOVE_TEXT = "Move";
constexpr const char* ROTATE_TEXT = "Rotate";
constexpr const char* SCALE_TEXT = "Scale";
constexpr const char* RECT_TRANSFORM_TEXT = "RectTransform";
constexpr const char* TRANSFORM_TEXT = "Transform";

constexpr const char* BAR_FUNCTIONS[5] = { MOVE_TEXT, ROTATE_TEXT, SCALE_TEXT, RECT_TRANSFORM_TEXT, TRANSFORM_TEXT };

EditorToolbar::EditorToolbar() 
{
    m_moduleEditor = app->getEditorModule();
}

EditorToolbar::~EditorToolbar() 
{
}

void EditorToolbar::DrawCentered(float menuWidth) 
{
    float startX = ImGui::GetCursorPosX();
    float toolbarWidth = 6 * m_buttonWidth;
    float centerPos = (menuWidth - toolbarWidth) * 0.5f;
    ImGui::SetCursorPosX(centerPos);

    int selectedIndex = static_cast<int>(m_moduleEditor->getCurrentSceneTool());
    int selectedNavIndex = static_cast<int>(m_moduleEditor->getCurrentNavigationMode());

    ManageNavigationButton(selectedIndex, selectedNavIndex);
    ManagePositionButton(selectedIndex);

    DrawGizmoModeButton(menuWidth, startX);
}

void EditorToolbar::ManageNavigationButton(int selectedIndex, int selectedNavIndex) 
{
    CreateButton(selectedIndex, VIEW_NAV_MODES[selectedNavIndex], 0);
}

void EditorToolbar::ManagePositionButton(int selectedIndex) 
{
    for (int i = 1; i < 6; i++) 
    {
        CreateButton(selectedIndex, BAR_FUNCTIONS[i - 1], i);
    }
}

void EditorToolbar::CreateButton(int selectedIndex, const char* text, int index) 
{
    if (selectedIndex == index) 
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.55f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.65f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.45f, 1.0f, 1.0f));
    }
    if (ImGui::Button(text, ImVec2(m_buttonWidth, m_buttonHeight))) 
    {
        m_moduleEditor->setCurrentSceneTool(index);
    }

    if (selectedIndex == index) 
    {
        ImGui::PopStyleColor(3);
    }

    ImGui::SameLine();
}

void EditorToolbar::DrawGizmoModeButton(float menuWidth, float startX) 
{
    ImGui::SameLine();
    const char* gizmoModeLabel = m_moduleEditor->isGizmoLocal() ? "Local" : "World";
    const float gizmoButtonWidth = ImGui::CalcTextSize(gizmoModeLabel).x + (ImGui::GetStyle().FramePadding.x * 2.0f);
    ImGui::SetCursorPosX(startX + menuWidth - gizmoButtonWidth);
    if (ImGui::Button(gizmoModeLabel, ImVec2(gizmoButtonWidth, m_buttonHeight))) 
{
        m_moduleEditor->toggleGizmoMode();
    }
    ImGui::SameLine();
}
