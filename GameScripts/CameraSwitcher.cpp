#include "pch.h"
#include "CameraSwitcher.h"
#include "Keyboard.h"

CameraSwitcher::CameraSwitcher(GameObject* owner)
    : Script(owner)
{
}

void CameraSwitcher::Start()
{
    rebuildCameraList();
    syncCurrentIndexWithDefaultCamera();
    m_wasSwitchKeyPressed = false;
}

void CameraSwitcher::onAfterReferencesFixed()
{
    rebuildCameraList();
    syncCurrentIndexWithDefaultCamera();
    m_wasSwitchKeyPressed = false;
}

void CameraSwitcher::Update()
{
    rebuildCameraList();

    if (m_currentIndex < 0 || m_currentIndex >= (int)m_cameras.size())
    {
        syncCurrentIndexWithDefaultCamera();
    }

    const bool isSwitchKeyPressed = Input::isKeyDown((int)DirectX::Keyboard::Keys::T);

    if (isSwitchKeyPressed && !m_wasSwitchKeyPressed)
    {
        switchToNextCamera();
    }

    m_wasSwitchKeyPressed = isSwitchKeyPressed;
}

void CameraSwitcher::rebuildCameraList()
{
    m_cameras.clear();

    const int cameraCount = SceneAPI::countGameObjectsByComponent(ComponentType::CAMERA, true);

    std::vector<GameObject*> cameraList(cameraCount, nullptr);

    const int count = SceneAPI::findGameObjectsByComponent(ComponentType::CAMERA, cameraList.data(), cameraCount, true);

    for (int i = 0; i < count; ++i)
    {
        if (cameraList[i])
        {
            m_cameras.push_back(cameraList[i]);
        }
    }
}

void CameraSwitcher::syncCurrentIndexWithDefaultCamera()
{
    GameObject* defaultCameraObject = SceneAPI::getDefaultCameraGameObject();
    m_currentIndex = -1;

    for (int i = 0; i < (int)m_cameras.size(); ++i)
    {
        if (m_cameras[i] == defaultCameraObject)
        {
            m_currentIndex = i;
            return;
        }
    }
}

void CameraSwitcher::switchToNextCamera()
{
    if (m_cameras.empty())
    {
        return;
    }

    syncCurrentIndexWithDefaultCamera();

    if (m_currentIndex == -1)
    {
        m_currentIndex = 0;
    }
    else
    {
        m_currentIndex = (m_currentIndex + 1) % (int)m_cameras.size();
    }

    SceneAPI::setDefaultCameraByGameObject(m_cameras[m_currentIndex]);
}

IMPLEMENT_SCRIPT(CameraSwitcher)