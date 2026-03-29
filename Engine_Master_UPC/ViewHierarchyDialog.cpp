#include "Globals.h"
#include "ViewHierarchyDialog.h"

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"
#include "PrefabUI.h"

#include "GameObject.h"
#include "Transform.h"
#include "Scene.h"
#include "UIButton.h"
#include "UIImage.h"
#include "Quadtree.h"
#include "WindowHierarchy.h"
#include <LightComponent.h>

ViewHierarchyDialog::ViewHierarchyDialog(WindowHierarchy* hierarchy)
{
	m_editorModule = app->getModuleEditor();
    m_sceneModule = app->getModuleScene();

	m_hierarchy = hierarchy;

    domTree.SetObject();
}

void ViewHierarchyDialog::render() {

}

void ViewHierarchyDialog::renderHierarchyMenu(GameObject* gameObject)
{

    GameObject* selected = m_editorModule->getSelectedGameObject();
    bool hasSelection = selected != nullptr;

    if (ImGui::MenuItem("Rename", nullptr, false, hasSelection))
    {
        if (hasSelection)
        {
            m_hierarchy->startRename(selected);
        }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Copy", nullptr, false, hasSelection))
    {
        DEBUG_WARN("Option not implemented yet!");
        //copy(selected);
    }

    if (ImGui::MenuItem("Paste", nullptr, false, hasSelection and domTree.HasMember("gameObjects")))
    {
        DEBUG_WARN("Option not implemented yet!");
        //pasteOn(selected);
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Create Empty"))
    {
        if (!gameObject) m_sceneModule->getScene()->createGameObject();
        else m_hierarchy->addChildToPrefabRoot(gameObject);
    }

    if (ImGui::MenuItem("Create Camera"))
    {
        GameObject* camera;
        if (!gameObject) camera = m_sceneModule->getScene()->createGameObject();
        else camera = m_hierarchy->addChildToPrefabRoot(gameObject);
        
        camera->SetName("New Camera");
        camera->AddComponent(ComponentType::CAMERA);
    }

    if (ImGui::BeginMenu("Create Light"))
    {
        if (ImGui::MenuItem("Point"))
        {
            GameObject* light;
            if (!gameObject) light = m_sceneModule->getScene()->createGameObject();
            else light = m_hierarchy->addChildToPrefabRoot(gameObject);

            light->SetName("New Point Light");
            LightComponent* lightComp = static_cast<LightComponent*>(light->AddComponentWithUID(ComponentType::LIGHT, GenerateUID()) );
            lightComp->setTypePoint(10.f);
        }

        if (ImGui::MenuItem("Directional"))
        {
            GameObject* light;
            if (!gameObject) light = m_sceneModule->getScene()->createGameObject();
            else light = m_hierarchy->addChildToPrefabRoot(gameObject);

            light->SetName("New Directional Light");
            LightComponent* lightComp = static_cast<LightComponent*>(light->AddComponentWithUID(ComponentType::LIGHT, GenerateUID()) );
            lightComp->setTypeDirectional();
        }

        if (ImGui::MenuItem("Spot"))
        {
            GameObject* light;
            if (!gameObject) light = m_sceneModule->getScene()->createGameObject();
            else light = m_hierarchy->addChildToPrefabRoot(gameObject);

            light->SetName("New Spot Light");
            LightComponent* lightComp = static_cast<LightComponent*>(light->AddComponentWithUID(ComponentType::LIGHT, GenerateUID()) );
            lightComp->setTypeSpot(10.f, 20.f, 30.f);
        }

        ImGui::EndMenu();
    }

    if (ImGui::MenuItem("Create Model"))
    {
        GameObject* model;
        if (!gameObject) model = m_sceneModule->getScene()->createGameObject();
        else model = m_hierarchy->addChildToPrefabRoot(gameObject);

        model->SetName("New Model");
        model->AddComponent(ComponentType::MODEL);
    }

    if (ImGui::BeginMenu("Create UI"))
    {
        if (ImGui::MenuItem("Canvas"))
        {
            GameObject* canvas;
            if (!gameObject) canvas = m_sceneModule->getScene()->createGameObject();
            else canvas = m_hierarchy->addChildToPrefabRoot(gameObject);

            canvas->SetName("New Canvas");
            canvas->AddComponentWithUID(ComponentType::CANVAS, GenerateUID());
        }

        if (ImGui::MenuItem("Button"))
        {
            Scene* currentScene;
            GameObject* button;

            if (!gameObject) 
            {
                currentScene = m_sceneModule->getScene();
                button = currentScene->createGameObject();

            }
            else 
            {
                currentScene = app->getModuleEditor()->getPrefabSession()->m_isolatedScene;
                button = m_hierarchy->addChildToPrefabRoot(gameObject);
            }

            button->SetName("New Button");
            button->AddComponentWithUID(ComponentType::TRANSFORM2D, GenerateUID());
            
            UIButton* buttonComp = (UIButton*) button->AddComponentWithUID(ComponentType::UIBUTTON, GenerateUID());
            UIImage* imageComp = (UIImage*) button->AddComponentWithUID(ComponentType::UIIMAGE, GenerateUID());
            buttonComp->setTargetGraphic(imageComp);

            if (not canvasExists(currentScene)) 
            {
                GameObject* canvas;
                if (!gameObject) canvas = currentScene->createGameObject();
                else canvas = m_hierarchy->addChildToPrefabRoot(gameObject);
                
                canvas->SetName("New Canvas");
                canvas->AddComponentWithUID(ComponentType::CANVAS, GenerateUID());

                m_hierarchy->reparent(button, canvas);
            }
        }

        if (ImGui::MenuItem("Text"))
        {
            Scene* currentScene;
            GameObject* text;

            if (!gameObject)
            {
                currentScene = m_sceneModule->getScene();
                text = currentScene->createGameObject();

            }
            else
            {
                currentScene = app->getModuleEditor()->getPrefabSession()->m_isolatedScene;
                text = m_hierarchy->addChildToPrefabRoot(gameObject);
            }

            text->SetName("New Text");
            text->AddComponentWithUID(ComponentType::TRANSFORM2D, GenerateUID());
            text->AddComponentWithUID(ComponentType::UITEXT, GenerateUID());

            if (not canvasExists(currentScene))
            {
                GameObject* canvas;
                if (!gameObject) canvas = currentScene->createGameObject();
                else canvas = m_hierarchy->addChildToPrefabRoot(gameObject);

                canvas->SetName("New Canvas");
                canvas->AddComponentWithUID(ComponentType::CANVAS, GenerateUID());

                m_hierarchy->reparent(text, canvas);
            }
        }

        if (ImGui::MenuItem("Image"))
        {
            Scene* currentScene;
            GameObject* image;

            if (!gameObject)
            {
                currentScene = m_sceneModule->getScene();
                image = currentScene->createGameObject();

            }
            else
            {
                currentScene = app->getModuleEditor()->getPrefabSession()->m_isolatedScene;
                image = m_hierarchy->addChildToPrefabRoot(gameObject);
            }

            image->SetName("New Image");
            image->AddComponentWithUID(ComponentType::TRANSFORM2D, GenerateUID());
            image->AddComponentWithUID(ComponentType::UIIMAGE, GenerateUID());

            if (not canvasExists(currentScene))
            {
                GameObject* canvas;
                if (!gameObject) canvas = currentScene->createGameObject();
                else canvas = m_hierarchy->addChildToPrefabRoot(gameObject);

                canvas->SetName("New Canvas");
                canvas->AddComponentWithUID(ComponentType::CANVAS, GenerateUID());

                m_hierarchy->reparent(image, canvas);
            }
        }

        ImGui::EndMenu();
    }

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.3f, 0.3f, 1.f));
    if (ImGui::MenuItem("Delete", nullptr, false, hasSelection))
    {
        if (hasSelection)
        {
            m_editorModule->setSelectedGameObject(nullptr);
            m_sceneModule->getScene()->removeGameObject(selected->GetID()); // TO SEE REMOVE IN PREFAB MODE
        }
    }
    ImGui::PopStyleColor();
}

void ViewHierarchyDialog::renderContextMenu(GameObject* gameObject, bool prefabMode, bool isEditRoot)
{
    PrefabUI::drawNodeContextMenu(gameObject, prefabMode, isEditRoot);

    if (!prefabMode)
    {
        PrefabUI::drawPrefabSubMenu(gameObject, app->getModuleScene()->getScene());
        ImGui::Separator();

        GameObject* selected = m_editorModule->getSelectedGameObject();
        bool hasSelection = selected != nullptr;
        if (ImGui::MenuItem("Rename", nullptr, false, hasSelection))
        {
            if (hasSelection)
            {
                m_hierarchy->startRename(selected);
            }
        }

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.3f, 0.3f, 1.f));
        if (ImGui::MenuItem("Delete"))
        {
            UID id = gameObject->GetID();
            if (app->getModuleEditor()->getSelectedGameObject() == gameObject)
                app->getModuleEditor()->setSelectedGameObject(nullptr);
            app->getModuleScene()->getScene()->removeGameObject(id);
        }
        ImGui::PopStyleColor();
    }

}

void ViewHierarchyDialog::copy(GameObject* selected)
{
    domTree.SetObject(); // clear

    //rapidjson::Value gameObjectList = selected->getNewHierarchyJSON(domTree);
    //domTree.AddMember("gameObjects", gameObjectList, domTree.GetAllocator());
}

void ViewHierarchyDialog::pasteOn(GameObject* selected)
{
    const rapidjson::Value& gameObjectList = domTree["gameObjects"];
    GameObject* gameObject = rebuildGameObject(gameObjectList);

    m_hierarchy->reparent(gameObject, selected);
}

GameObject* ViewHierarchyDialog::rebuildGameObject(const rapidjson::Value& objectList)
{
    // Create all objects and components
    std::unordered_map<uint64_t, GameObject*> uidToGo;
    std::unordered_map<uint64_t, uint64_t> childToParent;

    std::vector<GameObject*> rootObjects; // this replicates SceneModule implementation of JSON loading

    for (auto& gameObjectJson : objectList.GetArray())
    {
        const uint64_t uid = gameObjectJson["UID"].GetUint64();
        const uint64_t transformUid = gameObjectJson["Transform"]["UID"].GetUint64();
        GameObject* gameObject = createGameObjectWithUID((UID)uid, (UID)transformUid, rootObjects);

        uint64_t parentUid = 0;
        gameObject->deserializeJSON(gameObjectJson, parentUid);

        uidToGo[uid] = gameObject;
        childToParent[uid] = parentUid;
    }

    // Parent Child linking
    for (const auto& childAndParent : childToParent)
    {
        const uint64_t childUid = childAndParent.first;
        const uint64_t parentUid = childAndParent.second;

        if (parentUid == 0) {
            continue;
        }

        GameObject* child = uidToGo[childUid];
        GameObject* parent = uidToGo[parentUid];

        child->GetTransform()->setRoot(parent->GetTransform());
        parent->GetTransform()->addChild(child);

        removeFromList(child, rootObjects);
    }

    return rootObjects[0];
}

void ViewHierarchyDialog::removeFromList(GameObject* obj, std::vector<GameObject*>& objects)
{
    auto it = std::remove(
        objects.begin(),
        objects.end(),
        obj);

    objects.erase(it, objects.end());
}

GameObject* ViewHierarchyDialog::createGameObjectWithUID(UID id, UID transformUID, std::vector<GameObject*>& rootObjects)
{
    auto newGameObject = std::make_unique<GameObject>(id, transformUID);
    GameObject* raw = newGameObject.get();

    raw->init();

    m_sceneModule->getScene()->addGameObject(std::move(newGameObject));
    rootObjects.push_back(raw);

    raw->onTransformChange();

    Quadtree* quadTree = m_sceneModule->getQuadtree();
    if (quadTree)
    {
        quadTree->getRoot().insert(*raw);
    }
    
    return raw;  
}

bool ViewHierarchyDialog::canvasExists(Scene* scene)
{
    std::vector<GameObject*> sceneObjects = scene->getAllGameObjects();

    for (GameObject* object : sceneObjects) 
    {
        if (object->GetComponent(ComponentType::CANVAS)) return true;
    }

    return false;
}
