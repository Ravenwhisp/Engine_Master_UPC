#include "pch.h"
#include "UIController.h"
#include "AssetId.h"
#include "PersistingCheckpointState.h"
#include "PersistingPowerupState.h"

IMPLEMENT_SCRIPT_FIELDS(UIController,
	SERIALIZED_COMPONENT_REF(m_menuLights, "Main Menu Lights", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_blackBg, "Black Background", ComponentType::TRANSFORM2D),
	SERIALIZED_FLOAT(m_blackBgFadeDuration, "Black Background Fade Duration", 0.1f, 10.0f, 0.1f)
)

UIController::UIController(GameObject* owner): Script(owner) {}

void UIController::Start()
{
	Transform* menuLightsTransform = m_menuLights.getReferencedComponent();
	if (menuLightsTransform)
	{
		m_menuLightsGO = ComponentAPI::getOwner(menuLightsTransform);
	}
	m_blackBgTransform = m_blackBg.getReferencedComponent();
}
void UIController::Update()
{
	if (!m_isFading) return;

	m_blackBgFadeTimer -= Time::getDeltaTime();

	if (m_blackBgTransform && m_blackBgFadeDuration > 0.0f)
	{
		const float alpha = 1 - (m_blackBgFadeTimer / m_blackBgFadeDuration);
		Transform2DAPI::setAlpha(m_blackBgTransform, alpha);
	}

	if (m_blackBgFadeTimer <= 0.0f)
	{
		m_isFading = false;
		ChangeScene(m_pendingSceneName);
	}
}

static const ScriptMethodInfo UIControllerMethods[] =
{
	{ "StartScene", nullptr, ScriptMethodParamType::String, "sceneName", [](Script* s, const void* param) { static_cast<UIController*>(s)->StartScene(*static_cast<const std::string*>(param)); } },
	{ "ChangeScene", nullptr, ScriptMethodParamType::String, "sceneName", [](Script* s, const void* param) { static_cast<UIController*>(s)->ChangeScene(*static_cast<const std::string*>(param)); } },
	{ "ChangeScene2", nullptr, ScriptMethodParamType::AssetId, "sceneName", [](Script* s, const void* param) { static_cast<UIController*>(s)->ChangeScene2(*static_cast<const AssetId*>(param)); } },
	{ "ChangeLevel", [](Script* s) { static_cast<UIController*>(s)->ChangeLevel(); }  },
	{ "ExitApplication", [](Script* s) { static_cast<UIController*>(s)->ExitApplication(); } },
	{ "PauseGame", nullptr, ScriptMethodParamType::Bool, "pause", [](Script* s, const void* param) { static_cast<UIController*>(s)->PauseGame(*static_cast<const bool*>(param)); } }
};

ScriptMethodList UIController::getExposedMethods() const
{
	return { UIControllerMethods, sizeof(UIControllerMethods) / sizeof(ScriptMethodInfo) };
}

void UIController::StartScene(const std::string& sceneName)
{
	GameObjectAPI::setActive(m_menuLightsGO, true);

	m_pendingSceneName = sceneName;
	m_blackBgFadeTimer = m_blackBgFadeDuration;
	m_isFading = true;
}

void UIController::ChangeScene(const std::string& sceneName)
{
	SceneAPI::requestSceneChange(sceneName.c_str());
}

void UIController::ChangeScene2(const AssetId& sceneID)
{
	SceneAPI::requestSceneChange(sceneID);
}

void UIController::ChangeLevel()
{
	switch (PersistingCheckpointState::Get().m_lastSceneId)
	{
	case SceneId::LEVEL1:
		SceneAPI::requestSceneChange("Level1");
		break;
	case SceneId::LEVEL2:
		SceneAPI::requestSceneChange("Level2");
		break;
	case SceneId::LEVEL3:
		SceneAPI::requestSceneChange("BossLevel");
		break;
	default:
		SceneAPI::requestSceneChange("Main_Menu");
		break;
	}
}

void UIController::ExitApplication()
{
	ApplicationAPI::quit();
}

void UIController::StartGame(const std::string& sceneName)
{
	PersistingPowerupState::reset();
	ChangeScene(sceneName);
}

void UIController::PauseGame(bool pause)
{
	Debug::log("Pausing game: %s", pause ? "true" : "false");
	Time::setTimeScale(pause ? 0.0f : 1.0f);
	Debug::log("Game %s", Time::getTimeScale() == 0.0f ? "paused" : "resumed");
}

IMPLEMENT_SCRIPT(UIController)