#pragma once

#include "EditorWindow.h"
#include "rapidjson/document.h"
#include "UID.h"

class ModuleEditor;
class ModuleScene;

class WindowHierarchy;
class GameObject;
class Scene;

class ViewHierarchyDialog
{
private:
    ModuleEditor* m_editorModule;
    ModuleScene* m_sceneModule;
    WindowHierarchy* m_hierarchy;

    rapidjson::Document domTree;

public:
    explicit ViewHierarchyDialog(WindowHierarchy* hierarchy);

    void renderHierarchyMenu(GameObject* gameObject);
    void renderContextMenu(GameObject* gameObject, bool prefabMode, bool isEditRoot);

private:
    void drawCreateItems(Scene* scene, GameObject* parent);
    void copy(GameObject* selected);
    void pasteOn(GameObject* selected);
    GameObject* rebuildGameObject(const rapidjson::Value& objectList);
    void removeFromList(GameObject* obj, std::vector<GameObject*>& objects);
    GameObject* createGameObjectWithUID(UID id, UID transformUID, std::vector<GameObject*>& rootObjects);
    bool canvasExists(Scene* scene);
};
