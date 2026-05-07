#include "Globals.h"
#include "ChangeScene.h"

#include "Application.h"
#include "ModuleScene.h"

#include "GameObject.h"
#include "UIButton.h"

ChangeScene::ChangeScene(UID id, GameObject* gameObject):
	Component(id, ComponentType::CHANGE_SCENE, gameObject)
{
	DEBUG_LOG("ChangeScene constructed at: %p", this);
}
ChangeScene::~ChangeScene()
{
}

std::unique_ptr<Component> ChangeScene::clone(GameObject* newOwner) const
{
	std::unique_ptr<ChangeScene> newComponent = std::make_unique<ChangeScene>(m_uuid, newOwner);

	newComponent->setActive(this->isActive());
	newComponent->m_sceneToLoad = m_sceneToLoad;

	return newComponent;
}

bool ChangeScene::init()
{

    return true;
}

void ChangeScene::drawUi()
{
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 180, 0, 255));
	ImGui::Text("[!!!] Deprecated Component");
	ImGui::PopStyleColor();
}

void ChangeScene::onChangeScene()
{
	DEBUG_LOG("Bound ChangeScene at: %p, sceneToLoad: %s", this, m_sceneToLoad.c_str());

	app->getModuleScene()->requestSceneChange(m_sceneToLoad);
}

rapidjson::Value ChangeScene::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value componentInfo(rapidjson::kObjectType);
	componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
	componentInfo.AddMember("ComponentType", int(ComponentType::CHANGE_SCENE), domTree.GetAllocator());
	componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

	rapidjson::Value sceneString(m_sceneToLoad.c_str(), domTree.GetAllocator());
	componentInfo.AddMember("SceneToLoad", sceneString, domTree.GetAllocator());

	return componentInfo;
}

bool ChangeScene::deserializeJSON(const rapidjson::Value& componentInfo)
{
	if (componentInfo.HasMember("SceneToLoad"))
	{
		m_sceneToLoad = componentInfo["SceneToLoad"].GetString();
	}
	return true;
}
