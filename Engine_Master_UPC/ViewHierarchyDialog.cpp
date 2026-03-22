#include "Globals.h"
#include "ViewHierarchyDialog.h"

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"

#include "GameObject.h"
#include "Transform.h"
#include "Scene.h"
#include "Canvas.h"
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

void ViewHierarchyDialog::render()
{
    GameObject* selected = m_editorModule->getSelectedGameObject();
    bool hasSelection = selected != nullptr;

    if (ImGui::MenuItem("Remove", nullptr, false, hasSelection))
    {
        if (hasSelection)
        {
            m_editorModule->setSelectedGameObject(nullptr);
            m_sceneModule->getScene()->removeGameObject(selected->GetID());
        }
    }

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
        m_sceneModule->getScene()->createGameObject();
    }

    if (ImGui::MenuItem("Create Camera"))
    {
		GameObject* camera = m_sceneModule->getScene()->createGameObject();
		camera->AddComponent(ComponentType::CAMERA);
    }

    if (ImGui::BeginMenu("Create Light"))
    {
        if (ImGui::MenuItem("Point"))
        {
            GameObject* light = m_sceneModule->getScene()->createGameObject();
            LightComponent* lightComp = (LightComponent*)light->AddComponentWithUID(ComponentType::LIGHT, GenerateUID());
            lightComp->setTypePoint(10.f);
        }

        if (ImGui::MenuItem("Directional"))
        {
            GameObject* light = m_sceneModule->getScene()->createGameObject();
            LightComponent* lightComp = (LightComponent*)light->AddComponentWithUID(ComponentType::LIGHT, GenerateUID());
            lightComp->setTypeDirectional();
        }

        if (ImGui::MenuItem("Spot"))
        {
            GameObject* light = m_sceneModule->getScene()->createGameObject();
            LightComponent* lightComp = (LightComponent*)light->AddComponentWithUID(ComponentType::LIGHT, GenerateUID());
            lightComp->setTypeSpot(10.f, 20.f, 30.f);
        }

        ImGui::EndMenu();
    }

    if (ImGui::MenuItem("Create Model"))
    {
        GameObject* model = m_sceneModule->getScene()->createGameObject();
        model->AddComponent(ComponentType::MODEL);
    }

    if (ImGui::BeginMenu("Create UI"))
    {
        if (ImGui::MenuItem("Canvas"))
        {
            GameObject* canvas = m_sceneModule->getScene()->createGameObject();
            canvas->AddComponentWithUID(ComponentType::CANVAS, GenerateUID());
        }

        if (ImGui::MenuItem("Button"))
        {
            Scene* currentScene = m_sceneModule->getScene();

            GameObject* button = currentScene->createGameObject();
            button->AddComponentWithUID(ComponentType::TRANSFORM2D, GenerateUID());
            
            UIButton* buttonComp = (UIButton*) button->AddComponentWithUID(ComponentType::UIBUTTON, GenerateUID());
            UIImage* imageComp = (UIImage*) button->AddComponentWithUID(ComponentType::UIIMAGE, GenerateUID());
            buttonComp->setTargetGraphic(imageComp);

            if (not canvasExists()) 
            {
                GameObject* canvas = currentScene->createGameObject();
                canvas->SetName("New Canvas");
                canvas->AddComponentWithUID(ComponentType::CANVAS, GenerateUID());

                canvas->GetTransform()->addChild(button);
                currentScene->removeFromRootList(button); // so that it is not shown twice in the hierarchy
            }
        }

        if (ImGui::MenuItem("Text"))
        {
            Scene* currentScene = m_sceneModule->getScene();

            GameObject* text = currentScene->createGameObject();
            text->AddComponentWithUID(ComponentType::TRANSFORM2D, GenerateUID());
            text->AddComponentWithUID(ComponentType::UITEXT, GenerateUID());

            if (not canvasExists())
            {
                GameObject* canvas = currentScene->createGameObject();
                canvas->SetName("New Canvas");
                canvas->AddComponentWithUID(ComponentType::CANVAS, GenerateUID());

                canvas->GetTransform()->addChild(text);
                currentScene->removeFromRootList(text); // same
            }
        }

        if (ImGui::MenuItem("Image"))
        {
            Scene* currentScene = m_sceneModule->getScene();

            GameObject* image = currentScene->createGameObject();
            image->AddComponentWithUID(ComponentType::TRANSFORM2D, GenerateUID());
            image->AddComponentWithUID(ComponentType::UIIMAGE, GenerateUID());

            if (not canvasExists())
            {
                GameObject* canvas = currentScene->createGameObject();
                canvas->SetName("New Canvas");
                canvas->AddComponentWithUID(ComponentType::CANVAS, GenerateUID());

                canvas->GetTransform()->addChild(image);
                currentScene->removeFromRootList(image); // same
            }
        }

        ImGui::EndMenu();
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

bool ViewHierarchyDialog::canvasExists()
{
    std::vector<GameObject*> sceneObjects = m_sceneModule->getScene()->getAllGameObjects();

    for (GameObject* object : sceneObjects) 
    {
        if (object->GetComponent(ComponentType::CANVAS)) return true;
    }

    return false;
}
