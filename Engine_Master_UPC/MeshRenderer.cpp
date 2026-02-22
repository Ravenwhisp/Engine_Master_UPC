#include "Globals.h"
#include "MeshRenderer.h"
#include <Transform.h>
#include "GameObject.h"

#include "Application.h"
#include "RenderModule.h"
#include "AssetsModule.h"


void MeshRenderer::addModel(ModelAsset& model)
{
    m_meshes.clear();
    m_materials.clear();
    m_materialIndexByUID.clear();

    Vector3 globalMin(FLT_MAX, FLT_MAX, FLT_MAX);
    Vector3 globalMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (const auto meshAsset : model.getMeshes())
    {
        Vector3 meshMin = meshAsset.getBoundsCenter() - meshAsset.getBoundsExtents();
        Vector3 meshMax = meshAsset.getBoundsCenter() + meshAsset.getBoundsExtents();

        globalMin.x = std::min(globalMin.x, meshMin.x);
        globalMin.y = std::min(globalMin.y, meshMin.y);
        globalMin.z = std::min(globalMin.z, meshMin.z);

        globalMax.x = std::max(globalMax.x, meshMax.x);
        globalMax.y = std::max(globalMax.y, meshMax.y);
        globalMax.z = std::max(globalMax.z, meshMax.z);

        auto mesh = std::make_unique<BasicMesh>(meshAsset);
        m_meshes.push_back(std::move(mesh));
    }

    uint32_t index = 0;
    for (const auto materialAsset : model.getMaterials())
    {
        m_materialIndexByUID[materialAsset.getId()] = index;
        auto material = std::make_unique<BasicMaterial>(materialAsset);
        m_materials.push_back(std::move(material));
        ++index;
    }

    m_boundingBox = Engine::BoundingBox(globalMin, globalMax);
    m_hasBounds = true;
}

#pragma region Loop functions

bool MeshRenderer::init()
{

    return true;
}

void MeshRenderer::render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix)
{
    Transform* transform = m_owner->GetTransform();
    Matrix mvp = (transform->getGlobalMatrix() * viewMatrix * projectionMatrix).Transpose();
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(UINT32), &mvp, 0);

    for (const auto& mesh : getMeshes())
    {
        const auto& submeshes = mesh->getSubmeshes();

        for (const Submesh& submesh : submeshes)
        {
            auto it = m_materialIndexByUID.find(submesh.materialId);
            if (it == m_materialIndexByUID.end())
            {
                continue;
            }

            uint32_t materialIndex = it->second;
            BasicMaterial* material = m_materials[materialIndex].get();

            ModelData modelData{};
            modelData.model = transform->getGlobalMatrix().Transpose();
            modelData.normalMat = transform->getNormalMatrix().Transpose();
            modelData.material = material->getMaterial();

            // The numbers of the Root Parameters Index are hardcoded right now, maybe implement it in a enum
            commandList->SetGraphicsRootConstantBufferView(2 ,app->getRenderModule()->allocateInRingBuffer(&modelData, sizeof(ModelData)));
            commandList->SetGraphicsRootDescriptorTable(4, m_materials[materialIndex]->getTexture()->getSRV().gpu);

            mesh->draw(commandList, submesh);
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

bool MeshRenderer::cleanUp()
{
    return true;
}
#pragma endregion

void MeshRenderer::drawUi()
{
    ImGui::Separator();

    ImGui::Button("Drop Here");

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
        {
            const UID* data = static_cast<const UID*>(payload->Data);
            ModelAsset*modelAsset = static_cast<ModelAsset*>(app->getAssetModule()->requestAsset(*data));
            addModel(*modelAsset);
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::Separator();

    // --- Info ---
    ImGui::Text("Meshes: %d", (int)m_meshes.size());
    ImGui::Text("Materials: %d", (int)m_materials.size());

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

void MeshRenderer::onTransformChange()
{
    m_boundingBox.update(m_owner->GetTransform()->getGlobalMatrix());
}