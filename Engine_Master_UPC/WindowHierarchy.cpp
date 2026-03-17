#include "Globals.h"
#include "WindowHierarchy.h"

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"
#include "PrefabUI.h"
#include "PrefabEditSession.h"
#include "PrefabManager.h"

#include "GameObject.h"

WindowHierarchy::WindowHierarchy()
{

}

void WindowHierarchy::render()
{
	if (!ImGui::Begin(getWindowName(), getOpenPtr(),
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::End();
		return;
	}

	PrefabEditSession* session = app->getModuleEditor()->getPrefabSession();
	const bool prefabMode = session && session->m_active;

	if (prefabMode)
	{
		std::string filenameStr = session->m_sourcePath.filename().string();
		const char* cstr = filenameStr.c_str();
		PrefabUI::drawModeHeader(cstr);

		if (ImGui::Button("Add Child Object"))
			addChildToPrefabRoot(session->m_rootObject);
		ImGui::SameLine();
		if (ImGui::Button("Remove Selected"))
			removeGameObject();
		ImGui::Separator();

		if (session->m_isolatedScene)
		{
			for (GameObject* go : session->m_isolatedScene->getRootObjects())
				createTreeNode(go, true);
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

		createTreeNode();
	}

	ImGui::End();
}

void WindowHierarchy::createTreeNode(GameObject* gameObject, bool prefabMode)
{
	Transform* transform = gameObject->GetTransform();
	const auto children = transform->getAllChildren();

	PrefabEditSession* session = app->getModuleEditor()->getPrefabSession();
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

	const char* label = (isEditRoot && session) ? session->m_sourcePath.filename().string().c_str() : gameObject->GetName().c_str();
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
		app->getModuleEditor()->setSelectedGameObject(gameObject);

	// --- Context menu ---
	if (ImGui::BeginPopupContextItem())
	{
		PrefabUI::drawNodeContextMenu(gameObject, prefabMode, isEditRoot);

		if (!prefabMode)
		{
			PrefabUI::drawPrefabSubMenu(gameObject, app->getModuleScene());
			ImGui::Separator();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.3f, 0.3f, 1.f));
			if (ImGui::MenuItem("Delete"))
			{
				UID id = gameObject->GetID();
				if (app->getModuleEditor()->getSelectedGameObject() == gameObject)
					app->getModuleEditor()->setSelectedGameObject(nullptr);
				app->getModuleScene()->removeGameObject(id);
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

		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREFAB_ASSET"))
		{
			const std::filesystem::path sourcePath(static_cast<const char*>(payload->Data));
			ModuleScene* scene = app->getModuleScene();
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
}


void WindowHierarchy::reparent(GameObject* child, GameObject* newParent)
{
	if (!child) return;

	Transform* childTransform = child->GetTransform();
	Transform* newParentTransform = newParent ? newParent->GetTransform() : nullptr;

	if (newParentTransform && newParentTransform->isDescendantOf(childTransform))
		return;

	PrefabEditSession* session = app->getModuleEditor()->getPrefabSession();
	ModuleScene* targetScene = (session && session->m_active && session->m_isolatedScene) ? session->m_isolatedScene.get() : app->getModuleScene();

	Matrix worldMatrix = childTransform->getGlobalMatrix();

	Transform* oldRoot = childTransform->getRoot();
	GameObject* oldParent = oldRoot ? oldRoot->getOwner() : nullptr;

	if (oldParent)
	{
		oldParent->GetTransform()->removeChild(child->GetID());
	}
	else
	{
		targetScene->removeFromRootList(child);
	}

	childTransform->setRoot(newParentTransform);

	if (newParent)
	{
		newParentTransform->addChild(child);
	}
	else
	{
		targetScene->addToRootList(child);
	}

	childTransform->setFromGlobalMatrix(worldMatrix);
}


void WindowHierarchy::createTreeNode()
{
	if (ImGui::TreeNodeEx(app->getModuleScene()->getName()))
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
				ModuleScene* scene = app->getModuleScene();
				GameObject* spawned = PrefabManager::instantiatePrefab(sourcePath, scene);
				if (spawned)
					app->getModuleEditor()->setSelectedGameObject(spawned);
			}
			ImGui::EndDragDropTarget();
		}

		const auto& roots = app->getModuleScene()->getRootObjects();
		for (GameObject* gameObject : roots)
		{
			createTreeNode(gameObject, false);
		}

		ImGui::TreePop();
	}
}

void WindowHierarchy::addGameObject()
{
	app->getModuleScene()->createGameObject();
}

void WindowHierarchy::removeGameObject()
{
	GameObject* selected = app->getModuleEditor()->getSelectedGameObject();

	PrefabEditSession* session = app->getModuleEditor()->getPrefabSession();
	if (session && session->m_active && selected == session->m_rootObject)
	{
		return;
	}

	if (selected)
	{
		ModuleScene* targetScene = (session && session->m_active && session->m_isolatedScene) ? session->m_isolatedScene.get() : app->getModuleScene();

		UID id = selected->GetID();
		app->getModuleEditor()->setSelectedGameObject(nullptr);
		targetScene->removeGameObject(id);
	}
}

void WindowHierarchy::addChildToPrefabRoot(GameObject* parent)
{
	if (!parent) return;

	PrefabEditSession* session = app->getModuleEditor()->getPrefabSession();
	ModuleScene* targetScene = (session && session->m_active && session->m_isolatedScene)
		? session->m_isolatedScene.get()
		: app->getModuleScene();

	targetScene->createGameObject();

	const auto& roots = targetScene->getRootObjects();
	if (roots.empty()) return;

	GameObject* newObj = roots.back();
	if (!newObj || newObj == parent) return;

	reparent(newObj, parent);
	app->getModuleEditor()->setSelectedGameObject(newObj);
}
