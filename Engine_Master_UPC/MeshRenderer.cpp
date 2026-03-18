#include "Globals.h"
#include "MeshRenderer.h"
#include <Transform.h>
#include "GameObject.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleAssets.h"
#include "Settings.h"


std::unique_ptr<Component> MeshRenderer::clone(GameObject* newOwner) const
{
    std::unique_ptr<MeshRenderer> newMeshRenderer = std::make_unique<MeshRenderer>(m_uuid, newOwner);

    newMeshRenderer->m_modelAssetId = m_modelAssetId;
    newMeshRenderer->m_modelPath = m_modelPath;
    newMeshRenderer->m_basePath = m_basePath;
    newMeshRenderer->m_boundingBox = m_boundingBox;
    newMeshRenderer->m_meshes = m_meshes;
    newMeshRenderer->m_materials = m_materials;
    newMeshRenderer->m_materialIndexByUID = m_materialIndexByUID;  

    return newMeshRenderer;
}

void MeshRenderer::addModel(ModelAsset& model)
{
    m_meshes.clear();
    m_materials.clear();
    m_materialIndexByUID.clear();

    Vector3 globalMin(FLT_MAX, FLT_MAX, FLT_MAX);
    Vector3 globalMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    m_triangles = 0;

    for (const auto& meshAsset : model.getMeshes())
    {
        Vector3 meshMin = meshAsset.getBoundsCenter() - meshAsset.getBoundsExtents();
        Vector3 meshMax = meshAsset.getBoundsCenter() + meshAsset.getBoundsExtents();

        globalMin.x = std::min(globalMin.x, meshMin.x);
        globalMin.y = std::min(globalMin.y, meshMin.y);
        globalMin.z = std::min(globalMin.z, meshMin.z);

        globalMax.x = std::max(globalMax.x, meshMax.x);
        globalMax.y = std::max(globalMax.y, meshMax.y);
        globalMax.z = std::max(globalMax.z, meshMax.z);

        auto mesh = app->getModuleResources()->createMesh(meshAsset);

        if (!mesh)
        {
            continue;
        }

        for (const auto& submesh : mesh->getSubmeshes())
        {
            m_triangles += submesh.indexCount / 3;
        }

        m_meshes.push_back(std::move(mesh));
    }

    uint32_t index = 0;
    for (const auto materialAsset : model.getMaterials())
    {
        m_materialIndexByUID[materialAsset.getId()] = index;
        auto material = app->getModuleResources()->createMaterial(materialAsset);
        m_materials.push_back(std::move(material));
        ++index;
    }

    m_boundingBox = Engine::BoundingBox(globalMin, globalMax);
    m_boundingBox.update(m_owner->GetTransform()->getGlobalMatrix());
}

void MeshRenderer::drawUi()
{
    ImGui::Separator();

    ImGui::Button("Drop Here");

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
        {
            const UID* data = static_cast<const UID*>(payload->Data);
            m_modelAssetId = *data;
            ModelAsset* modelAsset = static_cast<ModelAsset*>(app->getAssetModule()->requestAsset(*data));
            addModel(*modelAsset);
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::Separator();

    // --- Info ---
    ImGui::Text("Meshes: %d", (int)m_meshes.size());
    ImGui::Text("Materials: %d", (int)m_materials.size());
    ImGui::Text("Triangles: %d", (int)m_triangles);

    auto min = m_boundingBox.getMin();
    auto max = m_boundingBox.getMax();
    ImGui::Text("Local Min: %.3f %.3f %.3f", min.x, min.y, min.z);
    ImGui::Text("Local Max: %.3f %.3f %.3f", max.x, max.y, max.z);
}

void MeshRenderer::onTransformChange()
{
    m_boundingBox.update(m_owner->GetTransform()->getGlobalMatrix());
}

rapidjson::Value MeshRenderer::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::MODEL), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("ModelAssetId", m_modelAssetId, domTree.GetAllocator());

    return componentInfo;
}

rapidjson::Value MeshRenderer::getNewJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", GenerateUID(), domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::MODEL), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("ModelAssetId", m_modelAssetId, domTree.GetAllocator());

    return componentInfo;
}

bool MeshRenderer::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("ModelAssetId"))
    {
        m_modelAssetId = componentInfo["ModelAssetId"].GetUint64();

        m_meshes.clear();
        m_materials.clear();
        m_materialIndexByUID.clear();

        ModelAsset* modelAsset = static_cast<ModelAsset*>(app->getAssetModule()->requestAsset(m_modelAssetId));
        if (modelAsset) {
            addModel(*modelAsset);
        }
    }

    return true;
}