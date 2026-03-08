#include "Globals.h"
#include "CameraSwitcher.h"

#include "Application.h"
#include "SceneModule.h"
#include "GameObject.h"
#include "CameraComponent.h"
#include "InputModule.h"

#include <imgui.h>

CameraSwitcher::CameraSwitcher(UID id, GameObject* gameObject)
    : Component(id, ComponentType::CAMERA_SWITCHER, gameObject)
{
}

std::unique_ptr<Component> CameraSwitcher::clone(GameObject* newOwner) const
{
    std::unique_ptr<CameraSwitcher> clonedComponent = std::make_unique<CameraSwitcher>(m_uuid, newOwner);
    clonedComponent->setActive(this->isActive());
    clonedComponent->m_currentIndex = -1;
    clonedComponent->m_wasSwitchKeyPressed = false;
    clonedComponent->m_hasBuiltCameraList = false;
    return clonedComponent;
}

bool CameraSwitcher::init()
{
    return true;
}

void CameraSwitcher::update()
{
    if (!m_hasBuiltCameraList)
    {
        rebuildCameraList();
        syncCurrentIndexWithDefaultCamera();
        m_wasSwitchKeyPressed = false;
        m_hasBuiltCameraList = true;
    }

    if (m_currentIndex < 0 || m_currentIndex >= (int)m_cameras.size())
    {
        syncCurrentIndexWithDefaultCamera();
    }

    const bool isSwitchKeyPressed = app->getInputModule()->isKeyDown(Keyboard::Keys::T);

    if (isSwitchKeyPressed && !m_wasSwitchKeyPressed)
    {
        switchToNextCamera();
    }

    m_wasSwitchKeyPressed = isSwitchKeyPressed;
}

void CameraSwitcher::drawUi()
{
    ImGui::Text("Camera Switcher");
    ImGui::Separator();
    ImGui::Text("Switch Key: T");
}

rapidjson::Value CameraSwitcher::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::CAMERA_SWITCHER), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    return componentInfo;
}

void CameraSwitcher::rebuildCameraList()
{
    m_cameras.clear();

    for (GameObject* gameObject : app->getSceneModule()->getAllGameObjects())
    {
        if (!gameObject || !gameObject->GetActive())
        {
            continue;
        }

        CameraComponent* camera = gameObject->GetComponentAs<CameraComponent>(ComponentType::CAMERA);
        if (camera && camera->isActive())
        {
            m_cameras.push_back(camera);
        }
    }
}

void CameraSwitcher::syncCurrentIndexWithDefaultCamera()
{
    CameraComponent* defaultCamera = app->getSceneModule()->getDefaultCamera();
    m_currentIndex = -1;

    for (int i = 0; i < (int)m_cameras.size(); ++i)
    {
        if (m_cameras[i] == defaultCamera)
        {
            m_currentIndex = i;
            return;
        }
    }
}

void CameraSwitcher::switchToNextCamera()
{
    syncCurrentIndexWithDefaultCamera();

    if (m_currentIndex == -1)
    {
        m_currentIndex = 0;
    }
    else
    {
        m_currentIndex = (m_currentIndex + 1) % (int)m_cameras.size();
    }

    app->getSceneModule()->setDefaultCamera(m_cameras[m_currentIndex]);

    GameObject* owner = m_cameras[m_currentIndex] ? m_cameras[m_currentIndex]->getOwner() : nullptr;
}