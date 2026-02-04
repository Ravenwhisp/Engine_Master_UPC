#include "Globals.h"
#include "Hierarchy.h"
#include "GameObject.h"

#include "Scene.h"

#include "Application.h"
#include "RenderModule.h"

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

	//TODO: Load the assets by dragging it in the hierarchy
	m_scene = app->getRenderModule()->getScene();

	createTreeNode(m_scene);

	ImGui::End();
}

void Hierarchy::createTreeNode(GameObject* gameObject)
{
	//First check if the game object has children
	std::vector<GameObject*> children = gameObject->GetChildren();
	int gameObjectNodeFlag = 0;
	if (children.empty()) 
	{
		gameObjectNodeFlag = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	} 
	else 
	{
		gameObjectNodeFlag = ImGuiTreeNodeFlags_OpenOnArrow;
	}

	if (ImGui::TreeNodeEx(gameObject->GetName(), gameObjectNodeFlag)) 
	{

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
			// Notify all listeners about the selected game object
			for (auto event : m_onSelectedGameObject) 
			{
				event(gameObject);
			}
		}

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
				if (droppedObject != gameObject &&
					!gameObject->IsChildOf(droppedObject))
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
			for (GameObject* child : children) 
			{
				createTreeNode(child);
			}
		}
	}
}

void Hierarchy::reparent(GameObject* child, GameObject* newParent)
{
	GameObject* oldParent = child->GetParent();

	if (oldParent)
	{
		oldParent->RemoveChild(child);
	}
	else
	{
		m_scene->remove(child);
	}

	child->SetParent(newParent);

	if (newParent)
	{
		newParent->AddChild(child);
	}
	else
	{
		m_scene->add(child);
	}
}

void Hierarchy::createTreeNode(Emeika::Scene* scene)
{
	if (ImGui::TreeNodeEx(scene->getName())) 
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

		for (GameObject* gameObject : scene->getGameObjects()) 
		{
			createTreeNode(gameObject);
		}

		ImGui::TreePop();
	}
}

void Hierarchy::addGameObject()
{
	auto gameObjects = m_scene->getGameObjects();
	int size = gameObjects.size();
	GameObject* gameObject = new GameObject(std::string("Game Object") + std::to_string(size));
	m_scene->add(gameObject);
}
