#pragma once

#include "EditorWindow.h"
#include "rapidjson/document.h"
#include "UID.h"

class EditorModule;
class SceneModule;

class Hierarchy;
class GameObject;

class ViewHierarchyDialog : public EditorWindow
{
private:
	EditorModule* m_editorModule;
	SceneModule* m_sceneModule;

	Hierarchy* m_hierarchy;

	rapidjson::Document domTree; // for saving JSON data

public:
	ViewHierarchyDialog(Hierarchy* hierarchy);

	void		render() override;
	const char* getWindowName() const override { return "HierarchyDialog"; }

private:

	void copy(GameObject* selected);
	void pasteOn(GameObject* selected);

	GameObject* rebuildGameObject(const rapidjson::Value& objectList);
	void removeFromList(GameObject* obj, std::vector<GameObject*>& objects);
	GameObject* createGameObjectWithUID(UID id, UID transformUID, std::vector<GameObject*>& rootObjects);
};
