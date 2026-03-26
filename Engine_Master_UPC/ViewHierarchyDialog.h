#pragma once

#include "EditorWindow.h"
#include "rapidjson/document.h"
#include "UID.h"

class ModuleEditor;
class ModuleScene;

class WindowHierarchy;
class GameObject;
class Scene;

class ViewHierarchyDialog : public EditorWindow
{
private:
	ModuleEditor* m_editorModule;
	ModuleScene* m_sceneModule;

	WindowHierarchy* m_hierarchy;

	rapidjson::Document domTree; // for saving JSON data

public:
	ViewHierarchyDialog(WindowHierarchy* hierarchy);

	void render();
	void renderHierarchyMenu (GameObject* gameObject);
	void renderContextMenu(GameObject* gameObject, bool prefabMode, bool isEditRoot);
	const char* getWindowName() const override { return "HierarchyDialog"; }

private:

	void copy(GameObject* selected);
	void pasteOn(GameObject* selected);

	GameObject* rebuildGameObject(const rapidjson::Value& objectList);
	void removeFromList(GameObject* obj, std::vector<GameObject*>& objects);
	GameObject* createGameObjectWithUID(UID id, UID transformUID, std::vector<GameObject*>& rootObjects);

	bool canvasExists(Scene* scene);
};
