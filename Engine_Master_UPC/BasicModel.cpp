#include "Globals.h"
#include "BasicModel.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#define TINYGLTF_IMPLEMENTATION /* Only in one of the includes */
#pragma warning(push)
#pragma warning(disable : 4018) 
#pragma warning(disable : 4267) 
#include "tiny_gltf.h"
#pragma warning(pop)

#include "Application.h"
#include "RenderModule.h"
#include "GameObject.h"
#include "Transform.h"

BasicModel::~BasicModel()
{
    for (int i = 0; i < m_meshes.size(); i++) 
    {
        delete m_meshes[i];
        m_meshes[i] = nullptr;
    }

    for (int i = 0; i < m_materials.size(); i++) 
    {
        delete m_materials[i];
        m_materials[i] = nullptr;
    }

}
void BasicModel::load(const char* fileName, const char* basePath)
{
    m_modelPath = fileName;
    m_basePath = basePath;

	tinygltf::TinyGLTF gltfContext;
	tinygltf::Model model;
	std::string error, warning;
	bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, fileName);
	if (loadOk)
	{
        Vector3 minVector = Vector3(99999.0f, 99999.0f, 99999.0f);
        Vector3 maxVector = Vector3(-99999.0f, -99999.0f, -99999.0f);

        for (tinygltf::Material material : model.materials) 
        {
            BasicMaterial* myMaterial = new BasicMaterial;
            myMaterial->load(model, material.pbrMetallicRoughness, basePath);
            m_materials.push_back(myMaterial);
        }

        for (tinygltf::Mesh mesh : model.meshes) 
        {
            for (tinygltf::Primitive primitive : mesh.primitives) 
            {
                BasicMesh* myMesh = new BasicMesh;
                myMesh->load(model, mesh, primitive, minVector, maxVector);
                m_meshes.push_back(myMesh);
            }
        }

        m_boundingBox.setMin(minVector);
        m_boundingBox.setMax(maxVector);
        m_boundingBox.update(m_owner->GetTransform()->getGlobalMatrix());
	}
    else
    {
        LOG("Error loading %s: %s", fileName, error.c_str());
    }
}

#pragma region Loop functions
bool BasicModel::init()
{
    load("Assets/Models/Duck/Duck.gltf", "Assets/Models/Duck/");
    return true;
}

void BasicModel::render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix)
{
	Transform* transform = m_owner->GetTransform();
    Matrix mvp = (transform->getGlobalMatrix() * viewMatrix * projectionMatrix).Transpose();
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(UINT32), &mvp, 0);

    for (BasicMesh* mesh : m_meshes) {
        int32_t materialIndex = mesh->getMaterialIndex();

        // Check if material index is valid
        if (materialIndex >= 0 && materialIndex < m_materials.size()) {

            ModelData modelData;
            modelData.model = transform->getGlobalMatrix().Transpose();
            modelData.material = m_materials[materialIndex]->getMaterial();
            modelData.normalMat = transform->getNormalMatrix();

            commandList->SetGraphicsRootConstantBufferView(2, app->getRenderModule()->allocateInRingBuffer(&modelData, sizeof(ModelData)));
            commandList->SetGraphicsRootDescriptorTable(3, m_materials[materialIndex]->getTexture()->getSRV().gpu);

            mesh->draw(commandList);
        }
    }

    m_boundingBox.render();
}

bool BasicModel::cleanUp()
{
    return true;
}
#pragma endregion

void BasicModel::drawUi()
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
        // limpiar anterior
        for (auto mesh : m_meshes)
            delete mesh;
        m_meshes.clear();

        for (auto material : m_materials)
            delete material;
        m_materials.clear();

        if (!m_modelPath.empty())
            load(m_modelPath.c_str(), m_basePath.c_str());
    }

    ImGui::SameLine();

    if (ImGui::Button("Reload"))
    {
        if (!m_modelPath.empty())
        {
            for (auto mesh : m_meshes)
                delete mesh;
            m_meshes.clear();

            for (auto material : m_materials)
                delete material;
            m_materials.clear();

            load(m_modelPath.c_str(), m_basePath.c_str());
        }
    }

    ImGui::Separator();

    // --- Info ---
    ImGui::Text("Meshes: %d", (int)m_meshes.size());
    ImGui::Text("Materials: %d", (int)m_materials.size());
}

void BasicModel::onTransformChange() 
{
    m_boundingBox.update(m_owner->GetTransform()->getGlobalMatrix());
}