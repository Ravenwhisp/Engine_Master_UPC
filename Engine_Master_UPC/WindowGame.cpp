#include "Globals.h"
#include "WindowGame.h"
#include <imgui.h>

#include "Application.h"

#include "ModuleRender.h"
#include "PlayToolbar.h"

WindowGame::WindowGame()
{
    m_playToolbar = new PlayToolbar();
}

WindowGame::~WindowGame()
{
    delete m_playToolbar;
}

void WindowGame::render()
{
    if (!ImGui::Begin(getWindowName(), getOpenPtr(), ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    float toolbarWidth = ImGui::GetContentRegionAvail().x;
    m_playToolbar->DrawCentered(toolbarWidth);

    ImGui::NewLine();
    ImGui::Separator();

    ImVec2 windowPos = ImGui::GetWindowPos();
    m_windowX = windowPos.x;
    m_windowY = windowPos.y;

    ImVec2 contentRegion = ImGui::GetContentRegionAvail();

    if (contentRegion.x > 0 && contentRegion.y > 0)
    {
        resize(contentRegion);

        ImVec2 imageTopLeft = ImGui::GetCursorScreenPos();
        m_viewportX = imageTopLeft.x;
        m_viewportY = imageTopLeft.y;

        ImTextureID textureID = (ImTextureID)app->getModuleRender()->getGPUPlayScreenRT().ptr;
        ImGui::Image(textureID, m_size);

    }

    //ImGuizmo::SetOrthographic(false);
    //ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());

    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
    ImVec2 contentPos(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
    ImVec2 contentSize(contentMax.x - contentMin.x, contentMax.y - contentMin.y);

    //ImGuizmo::SetRect(contentPos.x, contentPos.y, contentSize.x, contentSize.y);
    //ImGuizmo::Enable(true);

    m_isViewportHovered = ImGui::IsWindowHovered();
    m_isViewportFocused = ImGui::IsWindowFocused();

    ImGui::End();
}


bool WindowGame::resize(ImVec2 contentRegion)
{
    if (abs(contentRegion.x - m_size.x) > 1.0f ||
        abs(contentRegion.y - m_size.y) > 1.0f)
    {
        setSize(contentRegion);
        return true;
    }
    return false;
}
