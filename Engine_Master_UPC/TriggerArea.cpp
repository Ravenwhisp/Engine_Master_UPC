#include "Globals.h"
#include "TriggerArea.h"

#include "Application.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "QuadNode.h"

TriggerArea::TriggerArea(UID id, GameObject* gameObject): Component(id, ComponentType::CHANGE_SCENE_ON_TRIGGER, gameObject)
{
	m_xWidth = m_zWidth = 2.f;
	m_sceneToLoad = "";
	m_object1 = m_object2 = 0;

	DEBUG_LOG("TriggerArea constructed at: %p", this);
}


std::unique_ptr<Component> TriggerArea::clone(GameObject* newOwner) const
{
	std::unique_ptr<TriggerArea> newComponent = std::make_unique<TriggerArea>(m_uuid, newOwner);

	newComponent->m_xWidth = m_xWidth;
	newComponent->m_zWidth = m_zWidth;


	newComponent->m_sceneToLoad = m_sceneToLoad;

	newComponent->m_object1 = m_object1;
	newComponent->m_object2 = m_object2;

	return newComponent;
}

void TriggerArea::drawUi()
{
	ImGui::DragFloat("X Width", &m_xWidth, 1.f, 0.0f, FLT_MAX, "%.3f");
	ImGui::DragFloat("Z Width", &m_zWidth, 1.f, 0.0f, FLT_MAX, "%.3f");

	ImGui::Separator();

	static char saveSceneBuffer[256];
	strcpy_s(saveSceneBuffer, m_sceneToLoad.c_str());

	if (ImGui::InputText("Scene Name##Save", saveSceneBuffer, IM_ARRAYSIZE(saveSceneBuffer)))
	{
		m_sceneToLoad = saveSceneBuffer;
	}

	ImGui::Separator();

	ImGui::InputScalar("Object 1 UID", ImGuiDataType_U64, &m_object1);
	ImGui::InputScalar("Object 2 UID", ImGuiDataType_U64, &m_object2);


}

void TriggerArea::update()
{
	Vector3 currentPosition = m_owner->GetTransform()->getPosition();
	BoundingRect triggerArea(currentPosition.x - m_xWidth/2, currentPosition.z - m_zWidth/2, m_xWidth, m_zWidth);

	GameObject* gameObject1 = app->getModuleScene()->getScene()->findGameObjectByUID(m_object1);
	if (gameObject1) 
	{

		if (triggerArea.contains(gameObject1->GetTransform()->getPosition()) )
		{
			onChangeScene();
			return;
		}
	}

	GameObject* gameObject2 = app->getModuleScene()->getScene()->findGameObjectByUID(m_object2);
	if (gameObject2)
	{

		if (triggerArea.contains(gameObject2->GetTransform()->getPosition()))
		{
			onChangeScene();
		}
	}
}

/*void TriggerArea::preRender()
{
	printArea();
}
*/

void TriggerArea::onChangeScene()
{
	if (m_sceneToLoad == "") 
	{
		DEBUG_ERROR("Scene transition error at object %p: Scene not specified. Please specify a scene name", this);
		return;
	}

	DEBUG_LOG("Bound ChangeScene at: %p, sceneToLoad: %s", this, m_sceneToLoad.c_str());

	app->getModuleScene()->requestSceneChange(m_sceneToLoad);
}

void TriggerArea::printArea()
{
	Vector3 currentPosition = m_owner->GetTransform()->getPosition();

	ddVec3_In center = { currentPosition.x + m_xWidth / 2.f, currentPosition.y, currentPosition.z + m_zWidth / 2.f }; // half added because BoundingRect position was not the center, so we need to adjust on debug draw
	float height = 1.f; // this should go somewhere else...

	dd::box(center, dd::colors::Green, m_xWidth, height, m_zWidth);
}

rapidjson::Value TriggerArea::getJSON(rapidjson::Document& domTree) 
{

	rapidjson::Value componentInfo(rapidjson::kObjectType);
	componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
	componentInfo.AddMember("ComponentType", int(ComponentType::CHANGE_SCENE_ON_TRIGGER), domTree.GetAllocator());
	componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

	componentInfo.AddMember("XWidth", m_xWidth, domTree.GetAllocator());
	componentInfo.AddMember("ZWidth", m_zWidth, domTree.GetAllocator());
	{
		rapidjson::Value sceneString(m_sceneToLoad.c_str(), domTree.GetAllocator());
		componentInfo.AddMember("SceneToLoad", sceneString, domTree.GetAllocator());
	}
	
	componentInfo.AddMember("ObjectUID1", m_object1, domTree.GetAllocator());
	componentInfo.AddMember("ObjectUID2", m_object2, domTree.GetAllocator());

	return componentInfo;
}

bool TriggerArea::deserializeJSON(const rapidjson::Value& componentInfo)
{
	m_xWidth = componentInfo["XWidth"].GetFloat();
	m_zWidth = componentInfo["ZWidth"].GetFloat();

	if (componentInfo.HasMember("SceneToLoad"))
	{
		m_sceneToLoad = componentInfo["SceneToLoad"].GetString();
	}

	m_object1 = componentInfo["ObjectUID1"].GetUint64();
	m_object2 = componentInfo["ObjectUID2"].GetUint64();

	return true;
}
