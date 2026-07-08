#include "Globals.h"
#include "MeshRenderer.h"
#include "JsonArchive.h"

#include "Transform.h"
#include "GameObject.h"
#include <string>

#include "Application.h"
#include "ModuleAssets.h"
#include "ModuleResources.h"

#include "BasicMesh.h"
#include "MaterialAsset.h"
#include "SceneReferenceResolver.h"

MeshRenderer::~MeshRenderer() = default;

std::unique_ptr<Component> MeshRenderer::clone(GameObject* newOwner) const
{
    std::unique_ptr<MeshRenderer> newMeshRenderer = std::make_unique<MeshRenderer>(m_uuid, newOwner);

    newMeshRenderer->setActive(this->isActive());

    newMeshRenderer->m_meshAsset = m_meshAsset;
    newMeshRenderer->m_materialAssets = m_materialAssets;

    newMeshRenderer->m_skinAsset = m_skinAsset;

    if (m_skin)
    {
        newMeshRenderer->m_skin = m_skin->clone();
    }

    return newMeshRenderer;
}

void MeshRenderer::addMesh(MeshAsset& meshAsset)
{
    auto mesh = app->getModuleResources()->createMesh(meshAsset);
    if (mesh)
    {
        m_mesh = mesh;
        recompute();

        Vector3 boundsMin = meshAsset.getBoundsCenter() - meshAsset.getBoundsExtents();
        Vector3 boundsMax = meshAsset.getBoundsCenter() + meshAsset.getBoundsExtents();
        m_boundingBox = Engine::BoundingBox(boundsMin, boundsMax);
        m_boundingBox.update(m_owner->GetTransform()->getGlobalMatrix());
    }
}

void MeshRenderer::recompute()
{
    m_triangles = 0;
    if (m_mesh)
    {
        for (const auto& submesh : m_mesh->getSubmeshes())
            m_triangles += submesh.indexCount / 3;
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

    static const char* RENDER_TYPES[(int)RenderMode::COUNT] = { "Default", "Player" };
    int typeIndex = static_cast<int>(m_renderMode);
    if (ImGui::Combo("Render Mode", &typeIndex, RENDER_TYPES, (int)RenderMode::COUNT))
    {
        m_renderMode = static_cast<RenderMode>(typeIndex);
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

void MeshRenderer::serialize(IArchive& archive)
{
    Component::serialize(archive);

    if (archive.mode() == ArchiveMode::Input)
    {
        AssetReference ref;
        archive.beginObject("MeshAssetId");
        ref.serialize(archive);
        archive.endObject();
        setMeshReference(ref);

        archive.beginObject("SkinAssetId");
        ref.serialize(archive);
        archive.endObject();
        setSkinReference(ref);
        ensureSkin().setSkinReference(ref);

        uint32_t materialCount = 0;
        archive.beginArray(materialCount, "Materials");
        for (uint32_t i = 0; i < materialCount; ++i)
        {
            archive.beginObject();
            ref.serialize(archive);
            addMaterialReference(ref);
            archive.endObject();
        }
        archive.endArray();

        UINT renderMode = static_cast<UINT>(m_renderMode);
        archive.serialize(renderMode, "Render Mode");
        m_renderMode = static_cast<RenderMode>(renderMode);
    }
    else
    {
        archive.beginObject("MeshAssetId");
        m_meshAsset.serialize(archive);
        archive.endObject();

        archive.beginObject("SkinAssetId");
        m_skinAsset.serialize(archive);
        archive.endObject();

        uint32_t materialCount = static_cast<uint32_t>(m_materialAssets.size());
        archive.beginArray(materialCount, "Materials");

        for (uint32_t i = 0; i < materialCount; ++i)
        {
            archive.beginObject();
            m_materialAssets[i].serialize(archive);
            archive.endObject();
        }
        archive.endArray();

        UINT renderMode = static_cast<UINT>(m_renderMode);
        archive.serialize(renderMode, "Render Mode");
    }
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

void MeshRenderer::fixReferences(const SceneReferenceResolver& resolver)
{

    m_mesh = nullptr;
    m_materials.clear();


    if (m_meshAsset.isValid())
    {
        m_meshAsset.m_type = AssetType::MESH;
        auto meshAsset = app->getModuleAssets()->load<MeshAsset>(m_meshAsset);
        if (meshAsset)
            addMesh(*meshAsset);
    }

    for (auto& matRef : m_materialAssets)
    {
        if (matRef.isValid())
        {
            matRef.m_type = AssetType::MATERIAL;
            auto matAsset = app->getModuleAssets()->load<MaterialAsset>(matRef);
            if (matAsset)
                addMaterial(*matAsset);
        }
    }

    recompute();
}
