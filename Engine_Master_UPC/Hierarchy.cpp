#include "Globals.h"
#include "Hierarchy.h"
#include "GameObject.h"

#include "Scene.h"

#include "Application.h"
#include "RenderModule.h"

Hierarchy::Hierarchy()
{

}

void Hierarchy::Render()
{
	if (!ImGui::Begin(GetWindowName(), GetOpenPtr(),
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::End();
		return;
	}

	//Add gameObjects / filter by name
	if (ImGui::Button("+")) {
		AddGameObject();
	}

	ImGui::Separator();

	//TODO: Load the assets by dragging it in the hierarchy
	scene = app->getRenderModule()->getScene();

	CreateTreeNode(scene);

	ImGui::End();
}

void Hierarchy::CreateTreeNode(GameObject* gameObject)
{
	//First check if the game object has children
	std::vector<GameObject*> children = gameObject->GetChildren();
	int gameObjectNodeFlag = 0;
	if (children.empty()) {
		gameObjectNodeFlag = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	} else {
		gameObjectNodeFlag = ImGuiTreeNodeFlags_OpenOnArrow;
	}

	if (ImGui::TreeNodeEx(gameObject->GetName(), gameObjectNodeFlag)) {

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
			// Notify all listeners about the selected game object
			for (auto event : OnSelectedGameObject) {
				event(gameObject);
			}
		}

		//Drag source
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
					Reparent(droppedObject, gameObject);
				}
			}
			ImGui::EndDragDropTarget();
		}
		
		if(gameObjectNodeFlag != (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen))
		{
			ImGui::TreePop();
			//Create children nodes
			for (GameObject* child : children) {
				CreateTreeNode(child);
			}
		}
	}
}

void Hierarchy::Reparent(GameObject* child, GameObject* newParent)
{
	GameObject* oldParent = child->GetParent();

	// 1. Remove from old parent OR scene root
	if (oldParent)
	{
		oldParent->RemoveChild(child);
	}
	else
	{
		scene->remove(child);
	}

	// 2. Set new parent
	child->SetParent(newParent);

	// 3. Add to new parent OR scene root
	if (newParent)
	{
		newParent->AddChild(child);
	}
	else
	{
		scene->add(child);
	}
}

void Hierarchy::CreateTreeNode(Emeika::Scene* scene)
{
	if (ImGui::TreeNodeEx(scene->getName())) {

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAME_OBJECT"))
			{
				GameObject* droppedObject = *(GameObject**)payload->Data;
				Reparent(droppedObject, nullptr);
			}
			ImGui::EndDragDropTarget();
		}

		for (GameObject* gameObject : scene->getGameObjects()) {
			CreateTreeNode(gameObject);
		}

		ImGui::TreePop();
	}
}

void Hierarchy::AddGameObject()
{
	auto gameObjects = scene->getGameObjects();
	int size = gameObjects.size();
	GameObject* gameObject = new GameObject(std::string("Game Object") + std::to_string(size));
	scene->add(gameObject);
}
