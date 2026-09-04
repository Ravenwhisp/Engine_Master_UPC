#include "pch.h"
#include "UIController.h"
#include "PersistingPowerupState.h"

UIController::UIController(GameObject* owner): Script(owner) {}

void UIController::Start()
{
}

void UIController::Update()
{
}

static const ScriptMethodInfo UIControllerMethods[] =
{
	{ "ChangeScene", nullptr, ScriptMethodParamType::String, "sceneName", [](Script* s, const void* param) { static_cast<UIController*>(s)->ChangeScene(*static_cast<const std::string*>(param)); } },
	{ "ExitApplication", [](Script* s) { static_cast<UIController*>(s)->ExitApplication(); } },
	{ "PauseGame", nullptr, ScriptMethodParamType::Bool, "pause", [](Script* s, const void* param) { static_cast<UIController*>(s)->PauseGame(*static_cast<const bool*>(param)); } }
};

ScriptMethodList UIController::getExposedMethods() const
{
	return { UIControllerMethods, sizeof(UIControllerMethods) / sizeof(ScriptMethodInfo) };
}

void UIController::ChangeScene(const std::string& sceneName)
{
	SceneAPI::requestSceneChange(sceneName.c_str());
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