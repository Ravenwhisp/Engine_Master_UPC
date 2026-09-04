#include "pch.h"
#include "UIPause.h"

#include "PlayerController.h"

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
    Time::setTimeScale(1.0f);

    findPlayerControllers();

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
    setPlayersGameplayInputLocked(m_isPause);

    Transform* pausePanel = m_pausePanel.getReferencedComponent();
    if (pausePanel)
    {
        GameObjectAPI::setActive(ComponentAPI::getOwner(pausePanel), m_isPause && !m_isControlsOpen);
    }
}

void UIPause::findPlayerControllers()
{
    m_playerControllers.clear();

    const std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER, true);

    for (GameObject* player : players)
    {
        PlayerController* playerController = GameObjectAPI::findScript<PlayerController>(player);
        if (playerController == nullptr)
        {
            Debug::warn("UIPause could not find PlayerController on player '%s'.", GameObjectAPI::getName(player));
        }
        else
        {
            m_playerControllers.push_back(playerController);
        }
    }
}

void UIPause::setPlayersGameplayInputLocked(bool locked)
{
    for (PlayerController* playerController : m_playerControllers)
    {
        if (playerController == nullptr)
        {
            continue;
        }

        playerController->setGameplayInputLocked(locked);
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
