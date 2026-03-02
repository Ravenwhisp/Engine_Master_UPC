#include "Globals.h"
#include "ModelComponent.h"

#include "Application.h"
#include "RenderModule.h"
#include "GameObject.h"
#include "Transform.h"

#include "Logger.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#define TINYGLTF_IMPLEMENTATION /* Only in one of the includes */
#pragma warning(push)
#pragma warning(disable : 4018) 
#pragma warning(disable : 4267) 
#include "tiny_gltf.h"
#pragma warning(pop)


void ModelComponent::load(const char* fileName, const char* basePath)
{
    m_modelPath = fileName;
    m_basePath = basePath;
    std::string fullPath = std::string(m_basePath) + std::string(m_modelPath);

    if (app->getResourcesModule()->isModelLoaded(fullPath))
    {
        m_modelBinaryData = app->getResourcesModule()->getLoadedModel(fullPath).lock();
        // Since m_loadedModels has a weak_ptr, it can have expired references, so we need to check if there's actually data inside
        if (m_modelBinaryData)
        {
            std::string message = std::string("Model ") + fullPath + std::string(" already loaded in memory. Assigning meshes and materials to this model.");
            DEBUG_LOG(message.c_str());
            m_hasBounds = m_modelBinaryData->m_hasBounds;
            m_boundingBox = m_modelBinaryData->m_boundingBox;
            return;
        }
        else
        {
            if (app->getResourcesModule()->getLoadedModel(fullPath).expired())
            {
                app->getResourcesModule()->markModelAsNotLoaded(fullPath);
            }
            else
            {
                std::string error = std::string("Something is very wrong. Expected model ") + fullPath + std::string(" to be expired, but it's not, and lock() returned nullptr...");
                DEBUG_LOG(error.c_str());
            }
        }
    }

    m_hasBounds = false;

    m_boundingBox = Engine::BoundingBox(Vector3(FLT_MAX, FLT_MAX, FLT_MAX), Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX));

    tinygltf::TinyGLTF gltfContext;
    tinygltf::Model model;
    std::string error, warning;
    bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, fileName);
    if (loadOk)
    {
        m_modelBinaryData = std::make_shared<ModelBinaryData>();
        for (tinygltf::Material material : model.materials)
        {
            std::unique_ptr<BasicMaterial> myMaterial = std::make_unique<BasicMaterial>();
            if (!myMaterial->load(model, material.pbrMetallicRoughness, basePath)) {
                clearMaterials();
                return;
            }
            m_modelBinaryData->m_materials.push_back(std::move(myMaterial));
        }

        for (tinygltf::Mesh mesh : model.meshes)
        {
            for (tinygltf::Primitive primitive : mesh.primitives)
            {
                std::unique_ptr<BasicMesh> myMesh = std::make_unique<BasicMesh>();
                myMesh->load(model, mesh, primitive);

                if (myMesh->hasBounds())
                {
                    if (!m_hasBounds)
                    {
                        m_boundingBox.setMin(myMesh->getBoundsMin());
                        m_boundingBox.setMax(myMesh->getBoundsMax());
                        m_hasBounds = true;
                    }
                    else
                    {
                        const Vector3& mn = myMesh->getBoundsMin();
                        const Vector3& mx = myMesh->getBoundsMax();

                        Vector3 currentMin = m_boundingBox.getMin();
                        Vector3 currentMax = m_boundingBox.getMax();

                        Vector3 newMin(
                            std::min(currentMin.x, mn.x),
                            std::min(currentMin.y, mn.y),
                            std::min(currentMin.z, mn.z)
                        );

                        Vector3 newMax(
                            std::max(currentMax.x, mx.x),
                            std::max(currentMax.y, mx.y),
                            std::max(currentMax.z, mx.z)
                        );

                        m_boundingBox.setMin(newMin);
                        m_boundingBox.setMax(newMax);
                    }
                }

                m_modelBinaryData->m_meshes.push_back(std::move(myMesh));
            }
        }
        m_modelBinaryData->m_hasBounds = m_hasBounds;
        m_modelBinaryData->m_boundingBox = m_boundingBox;
        app->getResourcesModule()->markModelAsLoaded(fullPath, m_modelBinaryData);
    }
    else
    {

        DEBUG_ERROR("Error loading %s: %s", fileName, error.c_str());
    }
}

#pragma region Loop functions
bool ModelComponent::init()
{
    if (!m_modelPath.empty())
        load(m_modelPath.c_str(), m_basePath.c_str());     
    return true;
}

void ModelComponent::render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix)
{
    Transform* transform = m_owner->GetTransform();
    Matrix mvp = (transform->getGlobalMatrix() * viewMatrix * projectionMatrix).Transpose();
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(UINT32), &mvp, 0);

    for (auto& mesh : getMeshes()) {
        int32_t materialIndex = mesh->getMaterialIndex();

        // Check if material index is valid
        if (materialIndex >= 0 && materialIndex < m_modelBinaryData->m_materials.size()) {

            ModelData modelData;
            modelData.model = transform->getGlobalMatrix().Transpose();
            modelData.material = getMaterials()[materialIndex]->getMaterial();
            modelData.normalMat = transform->getNormalMatrix().Transpose();

            commandList->SetGraphicsRootConstantBufferView(2, app->getRenderModule()->allocateInRingBuffer(&modelData, sizeof(ModelData)));
            commandList->SetGraphicsRootDescriptorTable(4, getMaterials()[materialIndex]->getTexture()->getSRV().gpu);

            mesh->draw(commandList);
        }
    }

    if (m_drawBounds && m_hasBounds && dd::isInitialized())
    {
        const Matrix world = transform->getGlobalMatrix();

        const Vector3* c = m_boundingBox.getPoints();

        if (!m_drawWorldAabb)
        {
            ddVec3 pts[8];
            for (int i = 0; i < 8; ++i)
            {
                pts[i][0] = c[i].x; pts[i][1] = c[i].y; pts[i][2] = c[i].z;
            }
            dd::box(pts, dd::colors::Yellow, 0, m_boundsDepthTest);
        }
        else
        {
            Vector3 wmin(FLT_MAX, FLT_MAX, FLT_MAX);
            Vector3 wmax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
            for (int i = 0; i < 8; ++i)
            {
                wmin.x = std::min(wmin.x, c[i].x); wmin.y = std::min(wmin.y, c[i].y); wmin.z = std::min(wmin.z, c[i].z);
                wmax.x = std::max(wmax.x, c[i].x); wmax.y = std::max(wmax.y, c[i].y); wmax.z = std::max(wmax.z, c[i].z);
            }

            ddVec3 mn = { wmin.x, wmin.y, wmin.z };
            ddVec3 mx = { wmax.x, wmax.y, wmax.z };
            dd::aabb(mn, mx, dd::colors::Yellow, 0, m_boundsDepthTest);
        }
    }

}

bool ModelComponent::cleanUp()
{
    return true;
}
#pragma endregion

void ModelComponent::drawUi()
{
    ImGui::Separator();

    // --- Path fields ---
    char modelBuffer[256];
    std::strncpy(modelBuffer, m_modelPath.c_str(), sizeof(modelBuffer));
    modelBuffer[sizeof(modelBuffer) - 1] = '\0';

    if (ImGui::InputText("Model Path", modelBuffer, sizeof(modelBuffer)))
        m_modelPath = modelBuffer;

    char baseBuffer[256];
    std::strncpy(baseBuffer, m_basePath.c_str(), sizeof(baseBuffer));
    baseBuffer[sizeof(baseBuffer) - 1] = '\0';

    if (ImGui::InputText("Base Path", baseBuffer, sizeof(baseBuffer)))
        m_basePath = baseBuffer;

    ImGui::Spacing();

    // --- Buttons ---
    if (ImGui::Button("Load"))
    {
        m_hasBounds = false;

        // limpiar anterior
        clearMeshes();

        clearMaterials();

        if (!m_modelPath.empty())
        {
            load(m_modelPath.c_str(), m_basePath.c_str());
        }

    }

    ImGui::SameLine();

    if (ImGui::Button("Reload"))
    {
        m_hasBounds = false;

        if (!m_modelPath.empty())
        {
            clearMeshes();

            clearMaterials();

            load(m_modelPath.c_str(), m_basePath.c_str());
        }
    }

    ImGui::Separator();

    // --- Info ---
    ImGui::Text("Meshes: %d", (int)getMeshes().size());
    ImGui::Text("Materials: %d", (int)getMaterials().size());

    ImGui::SeparatorText("Debug Bounding Box");
    ImGui::Checkbox("Draw Bounding Box", &m_drawBounds);
    ImGui::Checkbox("Depth Test", &m_boundsDepthTest);
    ImGui::Checkbox("World AABB (axis aligned)", &m_drawWorldAabb);

    if (m_hasBounds)
    {
        auto min = m_boundingBox.getMin();
        auto max = m_boundingBox.getMax();
        ImGui::Text("Local Min: %.3f %.3f %.3f", min.x, min.y, min.z);
        ImGui::Text("Local Max: %.3f %.3f %.3f", max.x, max.y, max.z);
    }
    else
    {
        ImGui::TextDisabled("No bounds computed.");
    }

}

void ModelComponent::onTransformChange()
{
    m_boundingBox.update(m_owner->GetTransform()->getGlobalMatrix());
}

rapidjson::Value ModelComponent::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", unsigned int(ComponentType::MODEL), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());
    rapidjson::Value modelPath(m_modelPath.c_str(), domTree.GetAllocator());
    rapidjson::Value basePath(m_basePath.c_str(), domTree.GetAllocator());
    componentInfo.AddMember("ModelPath", modelPath, domTree.GetAllocator());
    componentInfo.AddMember("BasePath", basePath, domTree.GetAllocator());

    return componentInfo;
}

bool ModelComponent::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("ModelPath") && componentInfo.HasMember("BasePath"))
    {
        m_modelPath = componentInfo["ModelPath"].GetString();
        m_basePath = componentInfo["BasePath"].GetString();

        load(m_modelPath.c_str(), m_basePath.c_str());
    }

    return true;
}

