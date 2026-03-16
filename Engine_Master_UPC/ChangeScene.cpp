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
	if (m_uiButton)
	{
		m_uiButton->onClick.Remove(m_onClickHandle);
	}
}

std::unique_ptr<Component> ChangeScene::clone(GameObject* newOwner) const
{
	std::unique_ptr<ChangeScene> newComponent = std::make_unique<ChangeScene>(m_uuid, newOwner);

	newComponent->setActive(this->isActive());
	newComponent->m_sceneToLoad = m_sceneToLoad;
	newComponent->m_uiButton = m_uiButton;
	newComponent->m_uiButtonUid = m_uiButtonUid;

	return newComponent;
}

bool ChangeScene::init()
{

    return true;
}

void ChangeScene::drawUi()
{
	static char saveSceneBuffer[256];
	strcpy_s(saveSceneBuffer, m_sceneToLoad.c_str());

	if (ImGui::InputText("Scene Name##Save", saveSceneBuffer, IM_ARRAYSIZE(saveSceneBuffer)))
	{
		m_sceneToLoad = saveSceneBuffer;
	}

	ImGui::Separator();

	ImGui::Button("UI Button reference");

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT"))
		{
			Component* data = *static_cast<Component**>(payload->Data);
			UIButton* button = static_cast<UIButton*>(data);
			if (button)
			{
				if (m_uiButton)
				{
					m_uiButton->onClick.Remove(m_onClickHandle);
				}

				m_uiButton = button;
				m_uiButtonUid = button->getID();
				m_onClickHandle = m_uiButton->onClick.AddRaw(this, &ChangeScene::onChangeScene);
			}
		}
		ImGui::EndDragDropTarget();
	}

	// Target graphic slot
	ImGui::Text("UI BUtton: %s", m_uiButton ? "Assigned" : "None (drag a UI Button here)");
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

	componentInfo.AddMember("UIButtonUID", (uint64_t)m_uiButtonUid, domTree.GetAllocator());

	return componentInfo;
}

bool ChangeScene::deserializeJSON(const rapidjson::Value& componentInfo)
{
	if (componentInfo.HasMember("SceneToLoad"))
	{
		m_sceneToLoad = componentInfo["SceneToLoad"].GetString();
	}

	if (componentInfo.HasMember("UIButtonUID"))
	{
		m_uiButtonUid = (UID)componentInfo["UIButtonUID"].GetUint64();
	}

	m_uiButton = nullptr;
	return true;
}

void ChangeScene::fixReferences(const std::unordered_map<UID, Component*>& referenceMap)
{
	if (m_uiButton)
	{
		m_uiButton->onClick.Remove(m_onClickHandle);
		m_uiButton = nullptr;
	}

	if (m_uiButtonUid == 0)
	{
		return;
	}

	auto it = referenceMap.find(m_uiButtonUid);
	if (it != referenceMap.end())
	{
		m_uiButton = static_cast<UIButton*>(it->second);
		m_onClickHandle = m_uiButton->onClick.AddRaw(this, &ChangeScene::onChangeScene);
		DEBUG_LOG("Bound ChangeScene at: %p, sceneToLoad: %s", this, m_sceneToLoad.c_str());
	}
	else
	{
		DEBUG_LOG("Failed to fix reference for ChangeScene at: %p, UIButton UID not found", this);
		m_uiButton = nullptr;
	}
}
