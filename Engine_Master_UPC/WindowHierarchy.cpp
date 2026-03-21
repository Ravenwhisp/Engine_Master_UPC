#include "Globals.h"
#include "WindowHierarchy.h"

#include "ViewHierarchyDialog.h"

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"

#include "GameObject.h"

WindowHierarchy::WindowHierarchy()
{
	m_editorModule = app->getModuleEditor();
	m_sceneModule = app->getModuleScene();

	m_viewHierarchyDialog = new ViewHierarchyDialog(this);
}

void WindowHierarchy::render()
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

void WindowHierarchy::createTreeNode()
{
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;

	if (!m_editorModule->getSelectedGameObject())
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	bool opened = ImGui::TreeNodeEx(m_sceneModule->getName(), flags);

	if (ImGui::IsItemClicked())
	{
		m_editorModule->setSelectedGameObject(nullptr);
	}

	if (opened)
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

		const auto& roots = m_sceneModule->getRootObjects();
		for (GameObject* gameObject : roots)
		{
			createTreeNode(gameObject);
		}

		ImGui::TreePop();
	}
}

void WindowHierarchy::createTreeNode(GameObject* gameObject)
{
	Transform* transform = gameObject->GetTransform();
	const auto children = transform->getAllChildren();

	ImGuiTreeNodeFlags flags = children.empty() ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : ImGuiTreeNodeFlags_OpenOnArrow;

	if (gameObject == m_editorModule->getSelectedGameObject())
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	std::string label = gameObject->GetName() + "###" + std::to_string(gameObject->GetID());

	bool opened = false;

	if (m_renamingObject == gameObject)
	{
		ImGui::SetNextItemWidth(150);

		if (ImGui::InputText("##rename", m_renameBuffer, 256,
			ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
		{
			gameObject->SetName(m_renameBuffer);
			m_renamingObject = nullptr;
		}

		if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0))
		{
			gameObject->SetName(m_renameBuffer);
			m_renamingObject = nullptr;
		}
	}
	else
	{
		opened = ImGui::TreeNodeEx(label.c_str(), flags);
	}
	// --- Selection ---
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		//float time = ImGui::GetTime();

		if (m_editorModule->getSelectedGameObject() == gameObject && ImGui::IsMouseDoubleClicked(0) /*&& (time - m_lastClickTime) > 0.4f*/)
		{
			m_renamingObject = gameObject;
			strcpy(m_renameBuffer, gameObject->GetName().c_str());
		}

		m_editorModule->setSelectedGameObject(gameObject);
		//m_lastClickTime = time;
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


void WindowHierarchy::reparent(GameObject* child, GameObject* newParent)
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
		m_sceneModule->removeFromRootList(child);
	}

	childTransform->setRoot(newParentTransform);

	if (newParent)
	{
		newParentTransform->addChild(child);
	}
	else
	{
		m_sceneModule->addToRootList(child);
	}

	child->GetTransform()->setFromGlobalMatrix(child->GetTransform()->getGlobalMatrix());
}

void WindowHierarchy::startRename(GameObject* go)
{
	if (!go) return;

	m_renamingObject = go;
	strcpy(m_renameBuffer, go->GetName().c_str());
}