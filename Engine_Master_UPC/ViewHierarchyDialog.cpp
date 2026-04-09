#include "Globals.h"
#include "ViewHierarchyDialog.h"

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"
#include "PrefabUI.h"
#include "PrefabEditSession.h"

#include "GameObject.h"
#include "Transform.h"
#include "Scene.h"
#include "UIButton.h"
#include "UIImage.h"
#include "Quadtree.h"
#include "WindowHierarchy.h"
#include <LightComponent.h>

#include <CommandAddGameObject.h>
#include <CommandRemoveGameObject.h>
#include <HierarchyUtils.h>

ViewHierarchyDialog::ViewHierarchyDialog(WindowHierarchy* hierarchy)
{
    m_editorModule = app->getModuleEditor();
    m_sceneModule = app->getModuleScene();
    m_hierarchy = hierarchy;

    domTree.SetObject();
}

static Scene* resolveScene()
{
    return HierarchyUtils::resolveTargetScene();
}

static GameObject* createGO(Scene* scene, GameObject* parent = nullptr)
{
    CommandAddGameObject cmd(scene, parent);
    cmd.run();
    return cmd.getResult();
}

void ViewHierarchyDialog::renderHierarchyMenu(GameObject* gameObject)
{
    GameObject* selected = m_editorModule->getSelectedGameObject();
    bool hasSelect = selected != nullptr;

    if (ImGui::MenuItem("Rename", nullptr, false, hasSelect))
    {
        m_hierarchy->startRename(selected);
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Copy", nullptr, false, hasSelect))
    {
        DEBUG_WARN("Option not implemented yet!");
    }

    if (ImGui::MenuItem("Paste", nullptr, false, hasSelect && domTree.HasMember("gameObjects")))
    {
        DEBUG_WARN("Option not implemented yet!");
    }

    ImGui::Separator();

    PrefabEditSession* session = app->getModuleEditor()->getPrefabSession();
    Scene* scene = (gameObject && session && session->m_isolatedScene)
        ? session->m_isolatedScene
        : m_sceneModule->getScene();

    drawCreateItems(scene, nullptr);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.3f, 0.3f, 1.f));
    if (ImGui::MenuItem("Delete", nullptr, false, hasSelect))
    {
        m_editorModule->setSelectedGameObject(nullptr);
        CommandRemoveGameObject(resolveScene(), selected).run();
    }
    ImGui::PopStyleColor();
}

void ViewHierarchyDialog::renderContextMenu(GameObject* gameObject, bool prefabMode, bool)
{
    if (prefabMode)
    {
        return;
    }

    GameObject* selected = m_editorModule->getSelectedGameObject();
    bool hasSelect = selected != nullptr;

    if (ImGui::MenuItem("Rename", nullptr, false, hasSelect))
    {
        m_hierarchy->startRename(selected);
    }

    ImGui::Separator();

    Scene* scene = HierarchyUtils::resolveTargetScene();
    drawCreateItems(scene, gameObject);
}

void ViewHierarchyDialog::drawCreateItems(Scene* scene, GameObject* parent)
{
    if (ImGui::MenuItem("Create Empty"))
    {
        createGO(scene, parent);
    }

    if (ImGui::MenuItem("Create Camera"))
    {
        if (GameObject* camera = createGO(scene, parent))
        {
            camera->SetName("New Camera");
            camera->AddComponent(ComponentType::CAMERA);
        }
    }

    if (ImGui::BeginMenu("Create Light"))
    {
        if (ImGui::MenuItem("Point"))
        {
            if (GameObject* light = createGO(scene, parent))
            {
                light->SetName("New Point Light");
                auto* lc = static_cast<LightComponent*>(light->AddComponentWithUID(ComponentType::LIGHT, GenerateUID()));
                lc->setTypePoint(10.f);
            }
        }
        if (ImGui::MenuItem("Directional"))
        {
            if (GameObject* light = createGO(scene, parent))
            {
                light->SetName("New Directional Light");
                auto* lc = static_cast<LightComponent*>(light->AddComponentWithUID(ComponentType::LIGHT, GenerateUID()));
                lc->setTypeDirectional();
            }
        }
        if (ImGui::MenuItem("Spot"))
        {
            if (GameObject* light = createGO(scene, parent))
            {
                light->SetName("New Spot Light");
                auto* lc = static_cast<LightComponent*>(light->AddComponentWithUID(ComponentType::LIGHT, GenerateUID()));
                lc->setTypeSpot(10.f, 20.f, 30.f);
            }
        }
        ImGui::EndMenu();
    }

    if (ImGui::MenuItem("Create Model"))
    {
        if (GameObject* model = createGO(scene, parent))
        {
            model->SetName("New Model");
            model->AddComponent(ComponentType::MODEL);
        }
    }

    if (ImGui::BeginMenu("Create UI"))
    {
        if (ImGui::MenuItem("Canvas"))
        {
            if (GameObject* canvas = createGO(scene, parent))
            {
                canvas->SetName("New Canvas");
                canvas->AddComponentWithUID(ComponentType::CANVAS, GenerateUID());
            }
        }

        if (ImGui::MenuItem("Button"))
        {
            if (GameObject* button = createGO(scene, parent))
            {
                button->SetName("New Button");
                button->AddComponentWithUID(ComponentType::TRANSFORM2D, GenerateUID());
                auto* bc = static_cast<UIButton*>(button->AddComponentWithUID(ComponentType::UIBUTTON, GenerateUID()));
                auto* ic = static_cast<UIImage*>(button->AddComponentWithUID(ComponentType::UIIMAGE, GenerateUID()));
                bc->setTargetGraphic(ic);

                if (!canvasExists(scene))
                {
                    if (GameObject* canvas = createGO(scene, parent))
                    {
                        canvas->SetName("New Canvas");
                        canvas->AddComponentWithUID(ComponentType::CANVAS, GenerateUID());
                        m_hierarchy->reparent(button, canvas);
                    }
                }
            }
        }

        if (ImGui::MenuItem("Text"))
        {
            if (GameObject* text = createGO(scene, parent))
            {
                text->SetName("New Text");
                text->AddComponentWithUID(ComponentType::TRANSFORM2D, GenerateUID());
                text->AddComponentWithUID(ComponentType::UITEXT, GenerateUID());

                if (!canvasExists(scene))
                {
                    if (GameObject* canvas = createGO(scene, parent))
                    {
                        canvas->SetName("New Canvas");
                        canvas->AddComponentWithUID(ComponentType::CANVAS, GenerateUID());
                        m_hierarchy->reparent(text, canvas);
                    }
                }
            }
        }

        if (ImGui::MenuItem("Image"))
        {
            if (GameObject* image = createGO(scene, parent))
            {
                image->SetName("New Image");
                image->AddComponentWithUID(ComponentType::TRANSFORM2D, GenerateUID());
                image->AddComponentWithUID(ComponentType::UIIMAGE, GenerateUID());

                if (!canvasExists(scene))
                {
                    if (GameObject* canvas = createGO(scene, parent))
                    {
                        canvas->SetName("New Canvas");
                        canvas->AddComponentWithUID(ComponentType::CANVAS, GenerateUID());
                        m_hierarchy->reparent(image, canvas);
                    }
                }
            }
        }

        ImGui::EndMenu();
    }
}

void ViewHierarchyDialog::copy(GameObject*)
{
    domTree.SetObject();
}

void ViewHierarchyDialog::pasteOn(GameObject* selected)
{
    const rapidjson::Value& gameObjectList = domTree["gameObjects"];
    GameObject* gameObject = rebuildGameObject(gameObjectList);
    m_hierarchy->reparent(gameObject, selected);
}

GameObject* ViewHierarchyDialog::rebuildGameObject(const rapidjson::Value& objectList)
{
    std::unordered_map<uint64_t, GameObject*> uidToGo;
    std::unordered_map<uint64_t, uint64_t> childToParent;
    std::vector<GameObject*> rootObjects;

    for (auto& goJson : objectList.GetArray())
    {
        const uint64_t uid = goJson["UID"].GetUint64();
        const uint64_t transformUid = goJson["Transform"]["UID"].GetUint64();
        GameObject* go = createGameObjectWithUID((UID)uid, (UID)transformUid, rootObjects);

        uint64_t parentUid = 0;
        go->deserializeJSON(goJson, parentUid);

        uidToGo[uid] = go;
        childToParent[uid] = parentUid;
    }

    for (const auto& [childUid, parentUid] : childToParent)
    {
        if (parentUid == 0)
        {
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
    auto it = std::remove(objects.begin(), objects.end(), obj);
    objects.erase(it, objects.end());
}

GameObject* ViewHierarchyDialog::createGameObjectWithUID(UID id, UID transformUID, std::vector<GameObject*>& rootObjects)
{
    auto newGO = std::make_unique<GameObject>(id, transformUID);
    GameObject* raw = newGO.get();

    raw->init();

    m_sceneModule->getScene()->addGameObject(std::move(newGO));
    rootObjects.push_back(raw);
    raw->onTransformChange();

    if (Quadtree* qt = m_sceneModule->getQuadtree())
    {
        qt->getRoot().insert(*raw);
    }

    return raw;
}

bool ViewHierarchyDialog::canvasExists(Scene* scene)
{
    for (GameObject* go : scene->getAllGameObjects())
    {
        if (go && go->GetComponent(ComponentType::CANVAS))
        {
            return true;
        }
    }
    return false;
}
