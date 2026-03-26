#include "Globals.h"
#include "WindowInspector.h"

#include "Application.h"
#include "ModuleEditor.h"

#include "GameObject.h"
#include "PrefabUI.h"
#include "PrefabEditSession.h"

WindowInspector::WindowInspector()
{
}

void WindowInspector::drawInternal()
{
    PrefabEditSession* session = app->getModuleEditor()->getPrefabSession();
    const bool prefabMode = session && session->m_active;

    if (prefabMode)
    {
        PrefabUI::drawModeHeader(session->m_sourcePath.stem().string().c_str());
        PrefabUI::drawApplyRevertBar(ImGui::GetContentRegionAvail().x);
    }

    GameObject* selectedGameObject = app->getModuleEditor()->getSelectedGameObject();

    if (selectedGameObject)
    {
        selectedGameObject->drawUI();
    }
}
