#include "Globals.h"
#include "WindowHierarchy.h"

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"

#include "GameObject.h"
#include "PrefabManager.h"
#include "ModuleScene.h"

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

	createTreeNode();

	ImGui::End();
}

void WindowHierarchy::createTreeNode(GameObject* gameObject)
{
	Transform* transform = gameObject->GetTransform();
	const auto children = transform->getAllChildren();

	ImGuiTreeNodeFlags flags =
		children.empty()
		? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen
		: ImGuiTreeNodeFlags_OpenOnArrow;

	std::string label =
		gameObject->GetName() + "###" + std::to_string(gameObject->GetID());

	bool opened = ImGui::TreeNodeEx(label.c_str(), flags);

	// --- Selection ---
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		m_pendingSelection = gameObject;
		m_isDragging = false;
	}

	// --- Drag source ---
	if (ImGui::BeginDragDropSource())
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

		// A prefab asset dragged from the FileDialog: instantiate it and
		// parent the new root GO under this node.
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

			// Prefab dragged from FileDialog onto the scene root: spawn at root level.
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
			createTreeNode(gameObject);
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

