#include "Globals.h"
#include "WindowGame.h"

#include <imgui.h>

#include "Application.h"

#include "ModuleRender.h"
#include "ModuleResources.h"
#include "PlayToolbar.h"
#include "EditorToolbar.h"
#include "RenderSurface.h"
#include "Texture.h"
#include "ModuleScene.h"
#include "Scene.h"
#include "CameraComponent.h"

WindowGame::WindowGame()
{
    m_editorToolbar = new EditorToolbar();
    m_playToolbar = new PlayToolbar();
    m_surface.reset(app->getModuleResources()->createRenderSurface(m_size.x, m_size.y));
    app->getModuleRender()->registerViewport(m_surface.get(), ModuleRender::ViewportType::PLAY, m_size.x, m_size.y);
}

WindowGame::~WindowGame()
{
    app->getModuleRender()->unregisterViewport(m_surface.get());

    delete m_playToolbar;
}

void WindowGame::onBecameHidden()
{
    app->getModuleRender()->setViewportVisible(m_surface.get(), false);
}

void WindowGame::onBecameVisible()
{
    app->getModuleRender()->setViewportVisible(m_surface.get(), true);
}

void WindowGame::drawInternal()
{
    float toolbarWidth = ImGui::GetContentRegionAvail().x;
    m_playToolbar->DrawCentered(toolbarWidth);
    ImGui::NewLine();
    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.0f);
    m_editorToolbar->DrawCentered(toolbarWidth);
    ImGui::PopStyleVar();

    ImGui::Separator();

    // Push zero padding – exactly like SceneEditor
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    // Begin child window to isolate the viewport area
    ImGui::BeginChild("GameViewport", ImGui::GetContentRegionAvail(), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    if (viewportSize.x > 0 && viewportSize.y > 0)
    {
        // Resize the render surface and register the viewport
        if (abs(viewportSize.x - m_size.x) > 1.0f || abs(viewportSize.y - m_size.y) > 1.0f)
        {
            resize(viewportSize);
        }
    }

    // Store window coordinates if needed elsewhere
    ImVec2 windowPos = ImGui::GetWindowPos();
    m_windowX = windowPos.x;
    m_windowY = windowPos.y;
    m_viewportX = ImGui::GetCursorScreenPos().x;
    m_viewportY = ImGui::GetCursorScreenPos().y;
    m_size = viewportSize;

    // Get the texture ID and draw the image
    ImTextureID textureID = (ImTextureID)m_surface->getTexture(RenderSurface::COMPOSITE)->getSRV().gpu.ptr;
    ImGui::Image(textureID, viewportSize);

    // Update hover/focus states
    m_isViewportHovered = ImGui::IsItemHovered();
    m_isViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

bool WindowGame::resize(ImVec2 contentRegion)
{
    setSize(contentRegion);
    // If we want Game window resize to behave the same as the Editor one, uncomment this option. Test it, it's a little weird and not sure 
    // it's something we want, but I "fixed" it because it was in the "engine known issues"
    // app->getModuleScene()->getScene()->getDefaultCamera()->setAspectRatio(contentRegion.x / contentRegion.y);
    app->getModuleRender()->setViewportPendingResize(m_surface.get(), ModuleRender::ViewportType::PLAY, contentRegion.x, contentRegion.y);
    return true;
}
