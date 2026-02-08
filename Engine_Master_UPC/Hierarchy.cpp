#include "Globals.h"
#include "Hierarchy.h"
#include "GameObject.h"

#include "Application.h"
#include "RenderModule.h"
#include "SceneModule.h"
#include "EditorModule.h"

Hierarchy::Hierarchy()
{

}

void Hierarchy::render()
{
	if (!ImGui::Begin(getWindowName(), getOpenPtr(),
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::End();
		return;
	}

	//Add gameObjects / filter by name
	if (ImGui::Button("+")) 
	{
		addGameObject();
	}

	ImGui::Separator();

	createTreeNode();

	ImGui::End();
}

void Hierarchy::createTreeNode(GameObject* gameObject)
{
	//First check if the game object has children
	const std::vector<GameObject*>* children = gameObject->getChildList();
	int gameObjectNodeFlag = 0;
	if (children->empty()) 
	{
		gameObjectNodeFlag = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	} 
	else 
	{
		gameObjectNodeFlag = ImGuiTreeNodeFlags_OpenOnArrow;
	}

	std::string itemName = gameObject->GetName() + "###" + std::to_string(gameObject->GetID());
	if (ImGui::TreeNodeEx(itemName.c_str(), gameObjectNodeFlag))
	{
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
			for (auto event : m_onSelectedGameObject) 
			{
				event(gameObject);
			}
		}
		/*

		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("GAME_OBJECT",&gameObject,sizeof(GameObject*));
			ImGui::Text("%s", gameObject->GetName());
			ImGui::EndDragDropSource();
		}

		//Drop target
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAME_OBJECT"))
			{
				//Payoload object dropped in this target
				GameObject* droppedObject = *(GameObject**)payload->Data;

				// Safety checks
				if (droppedObject != gameObject && !gameObject->IsChildOf(droppedObject))
				{
					reparent(droppedObject, gameObject);
				}
			}
			ImGui::EndDragDropTarget();
		}
		
		if(gameObjectNodeFlag != (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen))
		{
			ImGui::TreePop();
			//Create children nodes
			for (GameObject* child : *children) 
			{
				createTreeNode(child);
			}
		}
		ImGui::TreePop();
		*/
	}
}

void Hierarchy::reparent(GameObject* child, GameObject* newParent)
{
	GameObject* oldParent = child->GetParent();

	if (oldParent)
	{
		//oldParent->RemoveChild(child);
	}
	else
	{
		//m_scene->remove(child);
	}

	//child->SetParent(newParent);

	if (newParent)
	{
		//newParent->AddChild(child);
	}
	else
	{
		//m_scene->add(child);
	}
}

void Hierarchy::createTreeNode()
{
	if (ImGui::TreeNodeEx(app->getSceneModule()->getName()))
	{

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAME_OBJECT"))
			{
				GameObject* droppedObject = *(GameObject**)payload->Data;
				reparent(droppedObject, nullptr);
			}
			ImGui::EndDragDropTarget();
		}

		const std::vector<GameObject*>& gameObjectList = app->getSceneModule()->getAllGameObjects();
		for (GameObject* gameObject : gameObjectList)
		{
			createTreeNode(gameObject);
		}

		ImGui::TreePop();
	}
}

void Hierarchy::addGameObject()
{
	app->getSceneModule()->CreateGameObject();
}
