#include "pch.h"
#include "UIPause.h"

IMPLEMENT_SCRIPT_FIELDS(UIPause,
    SERIALIZED_COMPONENT_REF(m_pausePanel, "Pause Panel", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_controlsPanel, "Controls Panel", ComponentType::TRANSFORM)
)

static const ScriptMethodInfo UIPauseMethods[] =
{
    { "Resume", [](Script* s) { static_cast<UIPause*>(s)->Resume(); } },
    { "ChangeScene", nullptr, ScriptMethodParamType::String, "sceneName", [](Script* s, const void* param) { static_cast<UIPause*>(s)->ChangeScene(*static_cast<const std::string*>(param)); } },
    { "ToggleControls", nullptr, ScriptMethodParamType::Bool, "isOpen", [](Script* s, const void* param) { static_cast<UIPause*>(s)->ToggleControls(*static_cast<const bool*>(param)); } }
};

ScriptMethodList UIPause::getExposedMethods() const
{
    return { UIPauseMethods, sizeof(UIPauseMethods) / sizeof(ScriptMethodInfo) };
}

UIPause::UIPause(GameObject* owner) : Script(owner)
{
}

void UIPause::Start()
{
    m_isPause = false;
    m_isControlsOpen = false;

    Transform* pausePanel = m_pausePanel.getReferencedComponent();
    if (pausePanel)
    {
        GameObjectAPI::setActive(ComponentAPI::getOwner(pausePanel), false);
    }
    
    Transform* controlsPanel = m_controlsPanel.getReferencedComponent();
    if (controlsPanel)
    {
        GameObjectAPI::setActive(ComponentAPI::getOwner(controlsPanel), false);
    }
}

void UIPause::Update()
{
    if (Input::isPauseJustPressed(0))
    {
        if (m_isPause)
        {
            if (m_isControlsOpen)
            {
                ToggleControls(false);
            }
            else
            {
                Resume();
            }
        }
        else
        {
            SetPauseState(true);
        }
    }
}

void UIPause::SetPauseState(bool isPaused)
{
    m_isPause = isPaused;
    
    Time::setTimeScale(m_isPause ? 0.0f : 1.0f);

    Transform* pausePanel = m_pausePanel.getReferencedComponent();
    if (pausePanel)
    {
        GameObjectAPI::setActive(ComponentAPI::getOwner(pausePanel), m_isPause && !m_isControlsOpen);
    }
}

void UIPause::Resume()
{
    SetPauseState(false);
}

void UIPause::ChangeScene(const std::string& sceneName)
{
    Time::setTimeScale(1.0f);
    SceneAPI::requestSceneChange(sceneName.c_str());
}

void UIPause::ToggleControls(bool isOpen)
{
    m_isControlsOpen = isOpen;
    Transform* controlsPanel = m_controlsPanel.getReferencedComponent();
    if (controlsPanel)
    {
        GameObjectAPI::setActive(ComponentAPI::getOwner(controlsPanel), m_isControlsOpen);
    }

    Transform* pausePanel = m_pausePanel.getReferencedComponent();
    if (pausePanel)
    {
        GameObjectAPI::setActive(ComponentAPI::getOwner(pausePanel), m_isPause && !m_isControlsOpen);
    }
}

IMPLEMENT_SCRIPT(UIPause)
