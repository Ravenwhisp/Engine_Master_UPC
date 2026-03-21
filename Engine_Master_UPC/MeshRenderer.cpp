#include "Globals.h"
#include "MeshRenderer.h"

#include "Transform.h"
#include "GameObject.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "ModuleResources.h"
#include "Settings.h"

#include "BasicMesh.h"
#include "MaterialAsset.h"


std::unique_ptr<Component> MeshRenderer::clone(GameObject* newOwner) const
{
    std::unique_ptr<MeshRenderer> newMeshRenderer = std::make_unique<MeshRenderer>(m_uuid, newOwner);

    return newMeshRenderer;
}

void MeshRenderer::addMesh(MeshAsset& meshAsset)
{

    auto mesh = app->getModuleResources()->createMesh(meshAsset);
    if (mesh)
    {
        for (const auto& submesh : mesh->getSubmeshes())
        {
            m_triangles += submesh.indexCount / 3;
        }

         Vector3 boundsMin = meshAsset.getBoundsCenter() - meshAsset.getBoundsExtents();
         Vector3 boundsMax = meshAsset.getBoundsCenter() + meshAsset.getBoundsExtents();
         m_boundingBox = Engine::BoundingBox(boundsMin, boundsMax);
         m_boundingBox.update(m_owner->GetTransform()->getGlobalMatrix());

         m_mesh = mesh;
    }
}

void MeshRenderer::addMaterial(MaterialAsset& materialAsset)
{
    auto material = app->getModuleResources()->createMaterial(materialAsset);
    if (material) {
        m_materials.push_back(material);
    }
}


void MeshRenderer::drawUi()
{
    ImGui::Separator();

    // --- Mesh drop target ---
    ImGui::Button("Drop Mesh Here");
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_MESH"))
        {
            const MD5Hash* id = static_cast<const MD5Hash*>(payload->Data);
            auto meshAsset = app->getModuleAssets()->load<MeshAsset>(*id);
            if (meshAsset)
                addMesh(*meshAsset);
        }
        ImGui::EndDragDropTarget();
    }

    // --- Material drop target ---
    ImGui::Button("Drop Material Here");
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_MATERIAL"))
        {
            const MD5Hash* id = static_cast<const MD5Hash*>(payload->Data);
            auto materialAsset = app->getModuleAssets()->load<MaterialAsset>(*id);
            if (materialAsset)
                addMaterial(*materialAsset);
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::Separator();

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

    componentInfo.AddMember("MeshAssetId", rapidjson::Value(m_meshAsset.c_str(), domTree.GetAllocator()), domTree.GetAllocator());

    {
        rapidjson::Value materialsData(rapidjson::kArrayType);

        for (const auto& materials : m_materialAssets)
        {
            materialsData.PushBack(rapidjson::Value(materials.c_str(), domTree.GetAllocator()), domTree.GetAllocator());
        }

        componentInfo.AddMember("MaterialAssetId", materialsData, domTree.GetAllocator());
    }

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
    if (componentInfo.HasMember("MeshAssetId"))
    {
        const MD5Hash meshId = componentInfo["MeshAssetId"].GetString();
        auto meshAsset = app->getModuleAssets()->load<MeshAsset>(meshId);
        if (meshAsset)
        {
            addMesh(*meshAsset);
        }
    }

    if (componentInfo.HasMember("MaterialAssetId"))
    {
        const auto& arr = componentInfo["MaterialAssetId"];

        for (auto& arrayStrings : arr.GetArray())
        {
            const MD5Hash materialId = arrayStrings.GetString();
            auto materialAsset = app->getModuleAssets()->load<MaterialAsset>(materialId);
            if (materialAsset)
            {
                addMaterial(*materialAsset);
            }
        }
    }

    return true;
}