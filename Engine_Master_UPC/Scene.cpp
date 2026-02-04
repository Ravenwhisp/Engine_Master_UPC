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
    transform->setScale(Vector3(0.01f, 0.01f, 0.01f));

    Model* model = new Model();
    model->load("Assets/Geometry/Duck/Duck.gltf", "Assets/Geometry/Duck/");
    duck->AddComponent<Model>(model);

    m_gameObjects.push_back(duck);

    GameObject* light = new GameObject("Light");
    light->AddComponent<Light>();

    m_gameObjects.push_back(light);
}

Emeika::Scene::~Scene()
{
    for (GameObject* gameObject : m_gameObjects) 
    {
        auto components = gameObject->GetComponents();
        for (Component* component : components) 
        {
            delete component;
        }
        delete gameObject;
    }
}

void Emeika::Scene::add(GameObject* gameObject)
{
    if (!gameObject) 
    {
        int size = m_gameObjects.size();
        gameObject = new GameObject(std::string("Game Object") + std::to_string(size));
    }
    m_gameObjects.emplace_back(gameObject);
}

void Emeika::Scene::remove(GameObject* gameObject)
{
    m_gameObjects.erase(std::remove(m_gameObjects.begin(), m_gameObjects.end(), gameObject), m_gameObjects.end());
}

void Emeika::Scene::render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix)
{
    for(GameObject* gameobject : m_gameObjects)
    {
        Transform* transform = gameobject->GetComponent<Transform>();
        Emeika::Model* model = gameobject->GetComponent<Emeika::Model>();

        if (!model || !transform) break;

        std::vector<Emeika::Mesh*> _meshes = model->getMeshes();
        std::vector<Emeika::Material*> _materials = model->getMaterials();

        Matrix mvp = (transform->getWorldMatrix() * viewMatrix * projectionMatrix).Transpose();
        commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(UINT32), &mvp, 0);

        for (Emeika::Mesh* mesh : _meshes) {
            int32_t materialIndex = mesh->getMaterialIndex();

            // Check if material index is valid
            if (materialIndex >= 0 && materialIndex < _materials.size()) 
            {

                ModelData modelData;
                modelData.model = transform->getWorldMatrix().Transpose();
                modelData.material = _materials[materialIndex]->getMaterial();
                modelData.normalMat = transform->getNormalMatrix();

                commandList->SetGraphicsRootConstantBufferView(2, app->getRenderModule()->allocateInRingBuffer(&modelData, sizeof(ModelData)));
                commandList->SetGraphicsRootDescriptorTable(3, _materials[materialIndex]->getTexture()->getSRV().gpu);

                mesh->draw(commandList);
            }
        }
    }
}

SceneData& Emeika::Scene::getData()
{
    Light* light = new Light();
    Transform* transform = new Transform();
    //Search for the first light component
    for(GameObject* gameObject : m_gameObjects)
    {
        auto component = gameObject->GetComponent<Light>();
        if (component) 
        {
            transform = gameObject->GetComponent<Transform>();
            light = component;
            break;
        }
    }

    m_sceneData.lightDirection = transform->getForward();
    m_sceneData.lightColor = light->getColour();
    m_sceneData.ambientColor = light->getAmbientColour();

    m_sceneData.lightDirection.Normalize();

    return m_sceneData;
}
