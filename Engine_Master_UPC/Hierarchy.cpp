#include "Globals.h"
#include "Hierarchy.h"

#include "Application.h"
#include "EditorModule.h"
#include "SceneModule.h"
#include "PrefabUI.h"
#include "PrefabEditSession.h"
#include "PrefabManager.h"

#include "GameObject.h"

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

	PrefabEditSession* session = app->getEditorModule()->getPrefabSession();
	const bool prefabMode = session && session->m_active;

	if (prefabMode)
	{
		PrefabUI::drawModeHeader(session->m_prefabName.c_str());

		if (ImGui::Button("Add Child Object"))
		{
			addChildToPrefabRoot(session->m_rootObject);
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove Selected"))
		{
			removeGameObject();
		}
		ImGui::Separator();

		if (session->m_rootObject)
			createTreeNode(session->m_rootObject, true);
	}
	else
	{
		if (ImGui::Button("New game object"))
		{
			addGameObject();
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove game object"))
		{
			removeGameObject();
		}

		ImGui::Separator();
	}

	if (prefabMode)
	{
		if (session->m_isolatedScene)
		{
			for (GameObject* go : session->m_isolatedScene->getRootObjects())
				createTreeNode(go, true);
		}
	}
	else
	{
		createTreeNode();
	}

	ImGui::End();
}

void Hierarchy::createTreeNode(GameObject* gameObject, bool prefabMode)
{
	Transform* transform = gameObject->GetTransform();
	const auto children = transform->getAllChildren();

	PrefabEditSession* session = app->getEditorModule()->getPrefabSession();
	const bool isEditRoot = prefabMode && session && gameObject == session->m_rootObject;
	const bool isPrefabInst = !isEditRoot && PrefabManager::isPrefabInstance(gameObject);

	ImGuiTreeNodeFlags flags =
		children.empty()
		? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen
		: ImGuiTreeNodeFlags_OpenOnArrow;

	if (isEditRoot)
		flags |= ImGuiTreeNodeFlags_DefaultOpen;

	if (isEditRoot)   ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.20f, 1.f));
	else if (isPrefabInst) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.75f, 1.0f, 1.f));

	const char* label = (isEditRoot && session) ? session->m_prefabName.c_str() : gameObject->GetName().c_str();
	std::string nodeId = std::string(label) + "###" + std::to_string(gameObject->GetID());

	bool opened = ImGui::TreeNodeEx(nodeId.c_str(), flags);

	if (isEditRoot || isPrefabInst) ImGui::PopStyleColor();

	// --- Selection ---
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		m_pendingSelection = gameObject;
		m_isDragging = false;
	}

	// --- Right-click selects before popup opens ---
	if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		app->getEditorModule()->setSelectedGameObject(gameObject);

	// --- Context menu ---
	if (ImGui::BeginPopupContextItem())
	{
		PrefabUI::drawNodeContextMenu(gameObject, prefabMode, isEditRoot);

		if (!prefabMode)
		{
			PrefabUI::drawPrefabSubMenu(gameObject, app->getSceneModule());
			ImGui::Separator();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.3f, 0.3f, 1.f));
			if (ImGui::MenuItem("Delete"))
			{
				UID id = gameObject->GetID();
				if (app->getEditorModule()->getSelectedGameObject() == gameObject)
					app->getEditorModule()->setSelectedGameObject(nullptr);
				app->getSceneModule()->removeGameObject(id);
			}
			ImGui::PopStyleColor();
		}

		ImGui::EndPopup();
	}

	// --- Drag source ---
	if (!isEditRoot && ImGui::BeginDragDropSource())
	{
		m_isDragging = true;
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

	if (m_pendingSelection == gameObject && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		if (!m_isDragging)
		{
			app->getEditorModule()->setSelectedGameObject(gameObject);
		}
		m_pendingSelection = nullptr;
	}

	// --- Draw children ---
	if (opened && !children.empty())
	{
		for (GameObject* child : children)
		{
			createTreeNode(child, prefabMode);
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
			createTreeNode(gameObject, false);
		}

		ImGui::TreePop();
	}
}

void Hierarchy::addGameObject()
{
	app->getSceneModule()->createGameObject();
}

void Hierarchy::removeGameObject()
{
	GameObject* selected = app->getEditorModule()->getSelectedGameObject();
	
	PrefabEditSession* session = app->getEditorModule()->getPrefabSession();
	if (session && session->m_active && selected == session->m_rootObject)
	{
		return;
	}

	if (selected)
	{
		UID id = selected->GetID();
		app->getEditorModule()->setSelectedGameObject(nullptr);
		app->getSceneModule()->removeGameObject(id);
	}
}

void Hierarchy::addChildToPrefabRoot(GameObject* parent)
{
	if (!parent)
	{
		return;
	}

	app->getSceneModule()->createGameObject();

	const auto& roots = app->getSceneModule()->getRootObjects();
	if (roots.empty())
	{
		return;
	}

	GameObject* newObj = roots.back();
	if (!newObj || newObj == parent)
	{
		return;
	}

	reparent(newObj, parent);

	app->getEditorModule()->setSelectedGameObject(newObj);
}