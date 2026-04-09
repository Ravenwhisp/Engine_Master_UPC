#include "Globals.h"
#include "WindowInspector.h"

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"

#include "GameObject.h"
#include "Scene.h"
#include "PrefabUI.h"
#include "PrefabEditSession.h"

WindowInspector::WindowInspector()
{
}

void WindowInspector::lockInspector(GameObject* go)
{
    m_isLocked = true;
    m_lockedGameObjectUID = go ? go->GetID() : 0;
}

void WindowInspector::unlockInspector()
{
    m_isLocked = false;
    m_lockedGameObjectUID = 0;
}

bool WindowInspector::isLocked() const
{
    return m_isLocked;
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

    ImGui::SameLine();
    if (ImGui::Button(m_isLocked ? "Unlock" : "Lock"))
    {
        if (m_isLocked)
        {
            unlockInspector();
        }
        else
        {
            GameObject* selected = app->getModuleEditor()->getSelectedGameObject();
            if (selected)
            {
                lockInspector(selected);
            }
        }
    }

    GameObject* displayObject = nullptr;

    if (m_isLocked && m_lockedGameObjectUID != 0)
    {
        Scene* scene = app->getModuleScene()->getScene();
        displayObject = scene->findGameObjectByUID(m_lockedGameObjectUID);
        if (!displayObject)
        {
            unlockInspector();
        }
    }
    else
    {
        displayObject = app->getModuleEditor()->getSelectedGameObject();
    }

    if (displayObject)
    {
        displayObject->drawUI();
    }
}
