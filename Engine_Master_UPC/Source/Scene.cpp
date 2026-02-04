#include "Globals.h"
#include "Scene.h"

#include "Model.h"
#include "Transform.h"
#include "Light.h"

#include "Application.h"
#include "RenderModule.h"


Emeika::Scene::Scene() 
{
    GameObject* duck = new GameObject("Duck");

    Transform* transform = duck->GetComponent<Transform>();
    transform->SetScale(Vector3(0.01f, 0.01f, 0.01f));

    Model* model = new Model();
    model->Load("Assets/Geometry/Duck/Duck.gltf", "Assets/Geometry/Duck/");
    duck->AddComponent<Model>(model);

    gameObjects.push_back(duck);


    GameObject* light = new GameObject("Light");
    light->AddComponent<Light>();

    gameObjects.push_back(light);
}

Emeika::Scene::~Scene()
{
    for (GameObject* gameObject : gameObjects) {
        auto components = gameObject->GetComponents();
        for (Component* component : components) {
            delete component;
        }
        delete gameObject;
    }
}

void Emeika::Scene::Add(GameObject* gameObject)
{
    if (!gameObject) {
        int size = gameObjects.size();
        gameObject = new GameObject(std::string("Game Object") + std::to_string(size));
    }
    gameObjects.emplace_back(gameObject);
}

void Emeika::Scene::Remove(GameObject* gameObject)
{
	gameObjects.erase(std::remove(gameObjects.begin(), gameObjects.end(), gameObject), gameObjects.end());
}

void Emeika::Scene::Render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix)
{
    for(GameObject* gameobject : gameObjects)
    {
        Transform* transform = gameobject->GetComponent<Transform>();
        Emeika::Model* model = gameobject->GetComponent<Emeika::Model>();

        if (!model || !transform) break;

        std::vector<Emeika::Mesh*> _meshes = model->GetMeshes();
        std::vector<Emeika::Material*> _materials = model->GetMaterials();

        Matrix mvp = (transform->GetWorldMatrix() * viewMatrix * projectionMatrix).Transpose();
        commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(UINT32), &mvp, 0);

        for (Emeika::Mesh* mesh : _meshes) {
            int32_t materialIndex = mesh->GetMaterialIndex();

            // Check if material index is valid
            if (materialIndex >= 0 && materialIndex < _materials.size()) {

                ModelData modelData;
                modelData.model = transform->GetWorldMatrix().Transpose();
                modelData.material = _materials[materialIndex]->GetMaterial();
                modelData.normalMat = transform->GetNormalMatrix();

                commandList->SetGraphicsRootConstantBufferView(2, app->GetRenderModule()->AllocateInRingBuffer(&modelData, sizeof(ModelData)));
                commandList->SetGraphicsRootDescriptorTable(3, _materials[materialIndex]->GetTexture()->SRV().gpu);

                mesh->Draw(commandList);
            }
        }
    }
}

SceneData& Emeika::Scene::GetData()
{
    Light* light = new Light();
    Transform* transform = new Transform();
    //Search for the first light component
    for(GameObject* gameObject : gameObjects)
    {
        auto component = gameObject->GetComponent<Light>();
        if (component) {
            transform = gameObject->GetComponent<Transform>();
            light = component;
            break;
        }
    }

    sceneData.lightDirection = transform->GetForward();
    sceneData.lightColor = light->GetColour();
    sceneData.ambientColor = light->GetAmbientColour();

    sceneData.lightDirection.Normalize();

    return sceneData;
}
