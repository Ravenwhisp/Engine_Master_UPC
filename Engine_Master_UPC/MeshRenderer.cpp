#include "Globals.h"
#include "MeshRenderer.h"

#include "Transform.h"
#include "GameObject.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "ModuleResources.h"

#include "BasicMesh.h"
#include "MaterialAsset.h"

MeshRenderer::~MeshRenderer() = default;

std::unique_ptr<Component> MeshRenderer::clone(GameObject* newOwner) const
{
    std::unique_ptr<MeshRenderer> newMeshRenderer = std::make_unique<MeshRenderer>(m_uuid, newOwner);

    newMeshRenderer->setActive(this->isActive());

    newMeshRenderer->m_mesh = m_mesh;
    newMeshRenderer->m_materials = m_materials;

    newMeshRenderer->m_meshAsset = m_meshAsset;
    newMeshRenderer->m_materialAssets = m_materialAssets;

    newMeshRenderer->m_skinAsset = m_skinAsset;

    if (m_skin)
    {
        newMeshRenderer->m_skin = m_skin->clone();
    }
    
    newMeshRenderer->m_triangles = m_triangles;

    newMeshRenderer->m_boundingBox = m_boundingBox;
    newMeshRenderer->m_boundingBox.update(newOwner->GetTransform()->getGlobalMatrix());

    return newMeshRenderer;
}

void MeshRenderer::addMesh(MeshAsset& meshAsset)
{
    auto mesh = app->getModuleResources()->createMesh(meshAsset);
    if (mesh)
    {
        m_triangles = 0;

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
    if (material) 
    {
        m_materials.push_back(material);
    }
}


void MeshRenderer::drawUi()
{
    ImGui::Separator();

    if(m_meshAsset.isValid())
    {
        ImGui::Text("Mesh: %s", std::to_string(m_meshAsset.m_uid).c_str());
    }
	else
    {
        ImGui::Text("Mesh: None");
    }

    // --- Mesh drop target ---
    ImGui::Button("Drop Mesh Here");
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_MESH"))
        {
            UID* ref = static_cast<UID*>(payload->Data);
            AssetReference* assetRef = app->getModuleAssets()->findReference(*ref);
            auto meshAsset = app->getModuleAssets()->load<MeshAsset>(*assetRef);
            if (meshAsset)
            {
                addMesh(*meshAsset);
            }
        }
        ImGui::EndDragDropTarget();
    }

    if(m_materialAssets.empty())
	{
		ImGui::Text("Materials: None");
    }
    else {
        ImGui::Text("Materials:");
        for (const auto& materialRef : m_materialAssets)
        {
            ImGui::BulletText("%s", std::to_string(materialRef.m_uid).c_str());
        }
    }

    if (m_skinAsset.isValid())
    {
        ImGui::Text("Skin: %s", std::to_string(m_meshAsset.m_uid).c_str());
    }
    else
    {
        ImGui::Text("Skin: None");
    }

    // --- Material drop target ---
    ImGui::Button("Drop Material Here");
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_MATERIAL"))
        {
            UID* ref = static_cast<UID*>(payload->Data);
            AssetReference* assetRef = app->getModuleAssets()->findReference(*ref);
            auto materialAsset = app->getModuleAssets()->load<MaterialAsset>(*assetRef);
            if (materialAsset)
            {
                addMaterial(*materialAsset);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::Separator();

    ImGui::Text("Triangles: %d", (int)m_triangles);

    auto min = m_boundingBox.getMin();
    auto max = m_boundingBox.getMax();
    ImGui::Text("Local Min: %.3f %.3f %.3f", min.x, min.y, min.z);
    ImGui::Text("Local Max: %.3f %.3f %.3f", max.x, max.y, max.z);

    if (m_skin)
    {
        ImGui::Separator();
        ImGui::Text("Skin");
        m_skin->drawUi();
    }

}

void MeshRenderer::debugDraw()
{
    m_boundingBox.setIsCulled(m_isCulled);
    m_boundingBox.render();
}

void MeshRenderer::onTransformChange()
{
    m_boundingBox.update(m_owner->GetTransform()->getGlobalMatrix());
}

void MeshRenderer::update()
{
    if (m_skin)
    {
        m_skin->lateUpdate(m_owner, *this);
    }
}

rapidjson::Value MeshRenderer::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::MODEL), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("MeshAssetId",m_meshAsset.getJson(domTree.GetAllocator()), domTree.GetAllocator());
    componentInfo.AddMember("SkinAssetId",m_skinAsset.getJson(domTree.GetAllocator()), domTree.GetAllocator());

    {
        rapidjson::Value materialsData(rapidjson::kArrayType);

        for (const auto& materials : m_materialAssets)
        {
            materialsData.PushBack(materials.getJson(domTree.GetAllocator()), domTree.GetAllocator());
        }

        componentInfo.AddMember("MaterialAssetId", materialsData, domTree.GetAllocator());
    }

    return componentInfo;
}

bool MeshRenderer::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("MeshAssetId"))
    {
        AssetReference meshId;
        if (!meshId.deserializeJson(componentInfo["MeshAssetId"]))
		{
			DEBUG_WARN("[MeshRenderer] Failed to deserialize MeshAssetId.");
			return false;
		}

        m_meshAsset = meshId;

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
            AssetReference materialId;
            if (!materialId.deserializeJson(arrayStrings))
			{
				DEBUG_WARN("[MeshRenderer] Failed to deserialize a MaterialAssetId.");
				continue;
			}

            m_materialAssets.push_back(materialId);

            auto materialAsset = app->getModuleAssets()->load<MaterialAsset>(materialId);
            if (materialAsset)
            {
                addMaterial(*materialAsset);
            }
        }
    }

    if (componentInfo.HasMember("SkinAssetId"))
    {
        m_skinAsset.deserializeJson(componentInfo["SkinAssetId"]);

        ensureSkin().setSkinReference(m_skinAsset);
    }

    return true;
}

void MeshRenderer::setMeshReference(AssetReference& meshRef)
{
    m_meshAsset = meshRef;
}

void MeshRenderer::addMaterialReference(AssetReference& materialRef)
{
    m_materialAssets.push_back(materialRef);
}


Skin& MeshRenderer::ensureSkin()
{
    if (!m_skin)
    {
        m_skin = std::make_unique<Skin>();

    }

    return *m_skin;
}

void MeshRenderer::clearSkin()
{
    if (m_skin)
    {
        m_skin->cleanUp();
    }

    m_skin.reset();
}
