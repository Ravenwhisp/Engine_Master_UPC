#include "Globals.h"
#include "WindowHierarchy.h"

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"
#include "ViewHierarchyDialog.h"
#include "PrefabUI.h"
#include "PrefabManager.h"

#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"

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
	
	bool contextMenuOpened = false;

	const bool prefabMode = m_editorModule->isInPrefabEditMode();

	if (prefabMode)
	{
		std::string filenameStr = m_editorModule->getPrefabEditSourcePath().filename().string();
		PrefabUI::drawModeHeader(filenameStr.c_str());

		if (ImGui::Button("Add Child Object"))
			addChildToPrefabRoot(m_editorModule->getPrefabEditRoot());
		ImGui::SameLine();
		if (ImGui::Button("Remove Selected"))
			removeGameObject();
		ImGui::Separator();

		Scene* isolatedScene = m_editorModule->getPrefabEditScene();
		if (isolatedScene)
		{
			for (GameObject* go : isolatedScene->getRootObjects())
				contextMenuOpened = contextMenuOpened || createTreeNode(go, true);
		}
	}
	else
	{
		if (ImGui::Button("New game object"))
			addGameObject();
		ImGui::SameLine();
		if (ImGui::Button("Remove game object"))
			removeGameObject();
		ImGui::Separator();

		contextMenuOpened = createTreeNode();
	}
	

	if (!contextMenuOpened && ImGui::IsWindowHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		ImGui::OpenPopup("HierarchyDialogPopup");
	}

	if (ImGui::BeginPopup("HierarchyDialogPopup"))
	{
		if (prefabMode) m_viewHierarchyDialog->renderHierarchyMenu(m_editorModule->getPrefabEditRoot());
		else m_viewHierarchyDialog->renderHierarchyMenu(nullptr);
		ImGui::EndPopup();
	}

	ImGui::End();
}

bool WindowHierarchy::createTreeNode(GameObject* gameObject, bool prefabMode)
{
	Transform* transform = gameObject->GetTransform();
	const auto children = transform->getAllChildren();

	GameObject* editRoot = m_editorModule->getPrefabEditRoot();
	const bool isEditRoot = prefabMode && (gameObject == editRoot);
	const bool isPrefabInst = !isEditRoot && gameObject->IsPrefabInstance();

	ImGuiTreeNodeFlags flags =
		children.empty()
		? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen
		: ImGuiTreeNodeFlags_OpenOnArrow;

	if (isEditRoot)
		flags |= ImGuiTreeNodeFlags_DefaultOpen;

	if (isEditRoot)   ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.20f, 1.f));
	else if (isPrefabInst) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.75f, 1.0f, 1.f));

	const char* label = isEditRoot ? m_editorModule->getPrefabEditSourcePath().filename().string().c_str() : gameObject->GetName().c_str();
	std::string nodeId = std::string(label) + "###" + std::to_string(gameObject->GetID());

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
		opened = ImGui::TreeNodeEx(nodeId.c_str(), flags);
	}


	if (isEditRoot || isPrefabInst) ImGui::PopStyleColor();

	
	// --- Selection ---

	if ((ImGui::IsItemClicked(0) || ImGui::IsItemClicked(1)) && !ImGui::IsItemToggledOpen())
	{
		//float time = ImGui::GetTime();

		if (m_editorModule->getSelectedGameObject() == gameObject && ImGui::IsMouseDoubleClicked(0) )
		{
			m_renamingObject = gameObject;
			strcpy(m_renameBuffer, gameObject->GetName().c_str());
		}

		m_editorModule->setSelectedGameObject(gameObject);
		//m_lastClickTime = time;
	}


	/*
	// --- Selection ---
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		m_pendingSelection = gameObject;
		m_isDragging = false;
	}

	// --- Right-click selects before popup opens ---
	if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		app->getModuleEditor()->setSelectedGameObject(gameObject);
	*/


	// --- Context menu ---
	bool contextMenuOpened = false;

	if (ImGui::BeginPopupContextItem())
	{
		m_viewHierarchyDialog->renderContextMenu(gameObject, prefabMode, isEditRoot);
		contextMenuOpened = true;
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

		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREFAB_ASSET"))
		{
			const std::filesystem::path sourcePath(static_cast<const char*>(payload->Data));
			Scene* scene = app->getModuleScene()->getScene();
			GameObject* spawned = PrefabManager::instantiatePrefab(sourcePath, scene);
			if (spawned)
			{
				reparent(spawned, gameObject);
				app->getModuleEditor()->setSelectedGameObject(spawned);
			}
		}
		ImGui::EndDragDropTarget();
	}

	if (m_pendingSelection == gameObject && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		if (!m_isDragging)
		{
			app->getModuleEditor()->setSelectedGameObject(gameObject);
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

	return contextMenuOpened;
}


void WindowHierarchy::startRename(GameObject* go)
{
	if (!go) return;

	m_renamingObject = go;
	strcpy(m_renameBuffer, go->GetName().c_str());
}

void WindowHierarchy::reparent(GameObject* child, GameObject* newParent)
{
	if (!child) return;

	Transform* childTransform = child->GetTransform();
	Transform* newParentTransform = newParent ? newParent->GetTransform() : nullptr;

	if (newParentTransform && newParentTransform->isDescendantOf(childTransform))
		return;

	// Use the isolated scene during prefab editing, main scene otherwise.
	Scene* targetScene = m_editorModule->getPrefabEditScene() ? m_editorModule->getPrefabEditScene() : app->getModuleScene()->getScene();

	Matrix worldMatrix = childTransform->getGlobalMatrix();

	Transform* oldRoot = childTransform->getRoot();
	GameObject* oldParent = oldRoot ? oldRoot->getOwner() : nullptr;

	if (oldParent)
	{
		oldParent->GetTransform()->removeChild(child->GetID());
	}
	else
	{
		app->getModuleScene()->getScene()->removeFromRootList(child);
	}

	childTransform->setRoot(newParentTransform);

	if (newParent)
	{
		newParentTransform->addChild(child);
	}
	else
	{
		app->getModuleScene()->getScene()->addToRootList(child);
	}

	childTransform->setFromGlobalMatrix(worldMatrix);
}


bool WindowHierarchy::createTreeNode()
{
	bool contextMenuOpened = false;

	if (ImGui::TreeNodeEx(app->getModuleScene()->getScene()->getName()))
	{

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAME_OBJECT"))
			{
				GameObject* droppedObject = *(GameObject**)payload->Data;
				reparent(droppedObject, nullptr);
			}

			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREFAB_ASSET"))
			{
				const std::filesystem::path sourcePath(static_cast<const char*>(payload->Data));
				Scene* scene = app->getModuleScene()->getScene();
				GameObject* spawned = PrefabManager::instantiatePrefab(sourcePath, scene);
				if (spawned)
					app->getModuleEditor()->setSelectedGameObject(spawned);
			}
			ImGui::EndDragDropTarget();
		}

		const auto& roots = app->getModuleScene()->getScene()->getRootObjects();
		for (GameObject* gameObject : roots)
		{
			contextMenuOpened = contextMenuOpened || createTreeNode(gameObject, false);
		}

		ImGui::TreePop();
	}

	return contextMenuOpened;
}

void WindowHierarchy::addGameObject()
{
	app->getModuleScene()->getScene()->createGameObject();
}

void WindowHierarchy::removeGameObject()
{
	GameObject* selected = m_editorModule->getSelectedGameObject();

	if (selected && m_editorModule->isInPrefabEditMode()
		&& selected == m_editorModule->getPrefabEditRoot())
	{
		return;
	}

	if (selected)
	{
		Scene* targetScene = m_editorModule->getPrefabEditScene()
			? m_editorModule->getPrefabEditScene()
			: app->getModuleScene()->getScene();

		UID id = selected->GetID();
		m_editorModule->setSelectedGameObject(nullptr);
		targetScene->removeGameObject(id);
	}
}

GameObject* WindowHierarchy::addChildToPrefabRoot(GameObject* parent)
{
	if (!parent) return nullptr;

	Scene* targetScene = m_editorModule->getPrefabEditScene() ? m_editorModule->getPrefabEditScene() : app->getModuleScene()->getScene();

	GameObject* newObj = targetScene->createGameObject();
	reparent(newObj, parent);
	m_editorModule->setSelectedGameObject(newObj);
	return newObj;
}
