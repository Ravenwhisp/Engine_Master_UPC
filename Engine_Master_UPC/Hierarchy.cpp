#include "Globals.h"
#include "Hierarchy.h"

#include "ViewHierarchyDialog.h"

#include "Application.h"
#include "EditorModule.h"
#include "SceneModule.h"

#include "GameObject.h"

Hierarchy::Hierarchy()
{
	m_viewHierarchyDialog = new ViewHierarchyDialog();
}

void Hierarchy::render()
{
	if (!ImGui::Begin(getWindowName(), getOpenPtr(),
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::End();
		return;
	}

	ImGui::Separator();

	createTreeNode();

	if (ImGui::IsWindowHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		ImGui::OpenPopup("HierarchyDialogPopup");
	}

	if (ImGui::BeginPopup("HierarchyDialogPopup"))
	{
		m_viewHierarchyDialog->render();
		ImGui::EndPopup();
	}
	ImGui::End();
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

		const auto& roots = app->getSceneModule()->getRootObjects();
		for (GameObject* gameObject : roots)
		{
			createTreeNode(gameObject);
		}

		ImGui::TreePop();
	}
}

void Hierarchy::createTreeNode(GameObject* gameObject)
{
	Transform* transform = gameObject->GetTransform();
	const auto children = transform->getAllChildren();

	ImGuiTreeNodeFlags flags = children.empty() ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : ImGuiTreeNodeFlags_OpenOnArrow;

	if (gameObject == app->getEditorModule()->getSelectedGameObject())
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	std::string label = gameObject->GetName() + "###" + std::to_string(gameObject->GetID());

	bool opened = ImGui::TreeNodeEx(label.c_str(), flags);
		
	// --- Selection ---
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		app->getEditorModule()->setSelectedGameObject(gameObject);
	}

	// --- Drag source ---
	if (ImGui::BeginDragDropSource())
	{
		ImGui::SetDragDropPayload("GAME_OBJECT", &gameObject, sizeof(GameObject*));
		ImGui::Text("%s", gameObject->GetName().c_str());
		ImGui::EndDragDropSource();
	}

	// --- Drop target ---
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAME_OBJECT"))
		{
			GameObject* droppedObject = *(GameObject**)payload->Data;

			if (droppedObject != gameObject)
			{
				reparent(droppedObject, gameObject);
			}
		}
		ImGui::EndDragDropTarget();
	}

	// --- Draw children ---
	if (opened && !children.empty())
	{
		for (GameObject* child : children)
		{
			createTreeNode(child);
		}
		ImGui::TreePop();
	}
}

void Hierarchy::reparent(GameObject* child, GameObject* newParent)
{
	if (!child) return;

	Transform* childTransform = child->GetTransform();
	Transform* newParentTransform = newParent ? newParent->GetTransform() : nullptr;

	if (newParentTransform && newParentTransform->isDescendantOf(childTransform))
		return;

	Transform* oldRoot = childTransform->getRoot();
	GameObject* oldParent = oldRoot ? oldRoot->getOwner() : nullptr;

	if (oldParent)
	{
		oldParent->GetTransform()->removeChild(child->GetID());
	}
	else
	{
		app->getSceneModule()->removeFromRootList(child);
	}

	childTransform->setRoot(newParentTransform);

	if (newParent)
	{
		newParentTransform->addChild(child);
	}
	else
	{
		app->getSceneModule()->addToRootList(child);
	}

	child->GetTransform()->setFromGlobalMatrix(child->GetTransform()->getGlobalMatrix());
}
