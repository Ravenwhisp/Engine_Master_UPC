#include "Globals.h"
#include "MeshRenderer.h"

#include "Transform.h"
#include "GameObject.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleAssets.h"
#include "ModuleResources.h"
#include "Settings.h"

#include "BasicMesh.h"
#include "MaterialAsset.h"
#include "SkinAsset.h"

namespace
{
    GameObject* FindHierarchyRoot(GameObject* go)
    {
        if (!go) return nullptr;

        GameObject* current = go;
        while (current)
        {
            Transform* parentTf = current->GetTransform()->getRoot();
            if (!parentTf)
                break;

            current = parentTf->getOwner();
        }

        return current;
    }

    GameObject* FindByNameRecursive(GameObject* go, const std::string& name)
    {
        if (!go) return nullptr;

        if (go->GetName() == name)
            return go;

        for (GameObject* child : go->GetTransform()->getAllChildren())
        {
            if (GameObject* found = FindByNameRecursive(child, name))
                return found;
        }

        return nullptr;
    }

    static float GetWeightComponent(const Vector4& w, int index)
    {
        switch (index)
        {
        case 0: return w.x;
        case 1: return w.y;
        case 2: return w.z;
        default: return w.w;
        }
    }

    Matrix BuildNormalMatrixFromSkinMatrix(const Matrix& skinMatrix)
    {
        Matrix normal = skinMatrix;
        normal.Translation(Vector3::Zero);
        normal = normal.Invert();
        normal = normal.Transpose();
        return normal;
    }

    uint32_t GetCurrentSkinningFrameIndex()
    {
        return app->getModuleD3D12()->getCurrentFrameIndex();
    }
}

std::unique_ptr<Component> MeshRenderer::clone(GameObject* newOwner) const
{
    std::unique_ptr<MeshRenderer> newMeshRenderer = std::make_unique<MeshRenderer>(m_uuid, newOwner);

    newMeshRenderer->setActive(this->isActive());

    newMeshRenderer->m_mesh = m_mesh;
    newMeshRenderer->m_materials = m_materials;

    newMeshRenderer->m_meshAsset = m_meshAsset;
    newMeshRenderer->m_materialAssets = m_materialAssets;

    newMeshRenderer->m_skinAsset = m_skinAsset;
    newMeshRenderer->m_skin.reset();
    newMeshRenderer->m_jointTransforms.clear();
    newMeshRenderer->m_matrixPalette.clear();
    newMeshRenderer->m_normalPalette.clear();
    newMeshRenderer->m_skinBindingsResolved = false;

    newMeshRenderer->m_triangles = m_triangles;
    newMeshRenderer->m_sourceVertices = m_sourceVertices;
    newMeshRenderer->m_skinnedVertices.clear();
    newMeshRenderer->m_skinnedVertexBuffer.reset();
    newMeshRenderer->invalidateGpuSkinningResources();
    newMeshRenderer->m_enableCpuSkinningFallback = m_enableCpuSkinningFallback;

    newMeshRenderer->m_boundingBox = m_boundingBox;
    newMeshRenderer->m_boundingBox.update(newOwner->GetTransform()->getGlobalMatrix());

    return newMeshRenderer;
}

void MeshRenderer::addMesh(MeshAsset& meshAsset)
{

    auto mesh = app->getModuleResources()->createMesh(meshAsset);
    if (mesh)
    {
        cacheSourceVertices(meshAsset);

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

    ImGui::Separator();
    ImGui::Text("Skin Asset: %s", m_skinAsset != INVALID_ASSET_ID ? m_skinAsset.c_str() : "None");
    ImGui::Text("Skin Loaded: %s", m_skin ? "Yes" : "No");
    ImGui::Text("Resolved Joints: %d", (int)m_jointTransforms.size());
    ImGui::Text("Palette Size: %d", (int)m_matrixPalette.size());

    ImGui::Text("Source Vertices: %d", (int)m_sourceVertices.size());
    ImGui::Text("CPU Skinned VB: %s", m_skinnedVertexBuffer ? "Yes" : "No");

    ImGui::Text("GPU Skinning Vertex Capacity: %d", (int)m_gpuSkinningVertexCapacity);
    ImGui::Text("GPU Palette Joint Capacity: %d", (int)m_gpuPaletteJointCapacity);
    ImGui::Text("GPU Skinning Resources: %s", hasGpuSkinningResources() ? "Yes" : "No");

    ImGui::Checkbox("Enable CPU Skinning Fallback", &m_enableCpuSkinningFallback);
}

void MeshRenderer::debugDraw()
{
    m_boundingBox.render();
}

void MeshRenderer::onTransformChange()
{
    m_boundingBox.update(m_owner->GetTransform()->getGlobalMatrix());
}

void MeshRenderer::update()
{
    PERF_LOGIC("MeshRenderer::update");

    if (m_skinAsset == INVALID_ASSET_ID)
        return;

    if (!ensureSkinLoaded())
        return;

    if (!m_skinBindingsResolved)
    {
        if (!resolveSkinBindings())
            return;
    }


    rebuildMatrixPalette();
    updateGpuPaletteBuffers();

    if (m_enableCpuSkinningFallback)
    {
        rebuildCpuSkinnedVertexBuffer();
    }
    else
    {
        m_skinnedVertices.clear();
        m_skinnedVertexBuffer.reset();
    }
}

rapidjson::Value MeshRenderer::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::MODEL), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("MeshAssetId", rapidjson::Value(m_meshAsset.c_str(), domTree.GetAllocator()), domTree.GetAllocator());
    componentInfo.AddMember("SkinAssetId", rapidjson::Value(m_skinAsset.c_str(), domTree.GetAllocator()), domTree.GetAllocator());

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

bool MeshRenderer::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("MeshAssetId"))
    {
        const MD5Hash meshId = componentInfo["MeshAssetId"].GetString();
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
            const MD5Hash materialId = arrayStrings.GetString();
            m_materialAssets.push_back(materialId);
            auto materialAsset = app->getModuleAssets()->load<MaterialAsset>(materialId);
            if (materialAsset)
            {
                addMaterial(*materialAsset);
            }
        }
    }

    if (componentInfo.HasMember("SkinAssetId") && componentInfo["SkinAssetId"].IsString())
    {
        m_skinAsset = componentInfo["SkinAssetId"].GetString();
    }
    else
    {
        m_skinAsset = INVALID_ASSET_ID;
    }

    invalidateSkinningRuntime();

    return true;
}

const VertexBuffer* MeshRenderer::getCurrentGpuSkinnedVertexBuffer() const
{
    const uint32_t frameIndex = GetCurrentSkinningFrameIndex();
    return m_gpuSkinnedVertexBuffers[frameIndex].get();
}

ID3D12Resource* MeshRenderer::getCurrentGpuSkinnedOutputResource() const
{
    const uint32_t frameIndex = GetCurrentSkinningFrameIndex();
    return m_gpuSkinnedVertexBuffers[frameIndex] ? m_gpuSkinnedVertexBuffers[frameIndex]->getD3D12Resource().Get() : nullptr;
}

ID3D12Resource* MeshRenderer::getCurrentGpuPaletteModelResource() const
{
    const uint32_t frameIndex = GetCurrentSkinningFrameIndex();
    return m_gpuPaletteModelBuffers[frameIndex].Get();
}

ID3D12Resource* MeshRenderer::getCurrentGpuPaletteNormalResource() const
{
    const uint32_t frameIndex = GetCurrentSkinningFrameIndex();
    return m_gpuPaletteNormalBuffers[frameIndex].Get();
}

bool MeshRenderer::hasGpuSkinningResources() const
{
    const uint32_t frameIndex = GetCurrentSkinningFrameIndex();
    return m_gpuSkinnedVertexBuffers[frameIndex] != nullptr
        && m_gpuPaletteModelBuffers[frameIndex] != nullptr
        && m_gpuPaletteNormalBuffers[frameIndex] != nullptr;
}

bool MeshRenderer::ensureSkinLoaded()
{
    if (m_skinAsset == INVALID_ASSET_ID)
        return false;

    if (m_skin)
        return true;

    auto skinAsset = app->getModuleAssets()->load<SkinAsset>(m_skinAsset);
    if (!skinAsset)
    {
        DEBUG_WARN("[MeshRenderer] Could not load SkinAsset '%s'.", m_skinAsset.c_str());
        return false;
    }

    m_skin = skinAsset;
    m_skinBindingsResolved = false;
    return true;
}

bool MeshRenderer::resolveSkinBindings()
{
    if (!m_owner || !m_skin)
        return false;

    GameObject* root = FindHierarchyRoot(m_owner);
    if (!root)
        return false;

    const auto& joints = m_skin->getJoints();

    m_jointTransforms.clear();
    m_jointTransforms.reserve(joints.size());

    for (const SkinJoint& joint : joints)
    {
        GameObject* jointGo = FindByNameRecursive(root, joint.nodeName);
        if (!jointGo || !jointGo->GetTransform())
        {
            DEBUG_WARN("[MeshRenderer] Joint '%s' not found while resolving skin '%s'.",
                joint.nodeName.c_str(),
                m_skin->getName().c_str());

            m_jointTransforms.clear();
            m_skinBindingsResolved = false;
            return false;
        }

        m_jointTransforms.push_back(jointGo->GetTransform());
    }

    m_matrixPalette.resize(joints.size(), Matrix::Identity);
    m_normalPalette.resize(joints.size(), Matrix::Identity);
    m_skinBindingsResolved = true;
    return true;
}

void MeshRenderer::rebuildMatrixPalette()
{
    if (!m_skin || !m_skinBindingsResolved)
        return;

    const auto& joints = m_skin->getJoints();
    const size_t count = std::min(joints.size(), m_jointTransforms.size());

    if (m_matrixPalette.size() != count)
        m_matrixPalette.resize(count, Matrix::Identity);

    if (m_normalPalette.size() != count)
        m_normalPalette.resize(count, Matrix::Identity);

    for (size_t i = 0; i < count; ++i)
    {
        Transform* jointTransform = m_jointTransforms[i];
        if (!jointTransform)
        {
            m_matrixPalette[i] = Matrix::Identity;
            m_normalPalette[i] = Matrix::Identity;
            continue;
        }

        const Matrix jointWorld = jointTransform->getGlobalMatrix();
        const Matrix skinMatrix = joints[i].inverseBindMatrix * jointWorld;

        m_matrixPalette[i] = skinMatrix;
        m_normalPalette[i] = BuildNormalMatrixFromSkinMatrix(skinMatrix);
    }
}

void MeshRenderer::invalidateSkinningRuntime()
{
    m_skin.reset();
    m_jointTransforms.clear();
    m_matrixPalette.clear();
    m_normalPalette.clear();
    m_skinBindingsResolved = false;

    m_skinnedVertices.clear();
    m_skinnedVertexBuffer.reset();

    invalidateGpuSkinningResources();
}

bool MeshRenderer::ensureGpuSkinningResources()
{
    if (!m_mesh || m_sourceVertices.empty())
        return false;

    const size_t vertexCount = m_sourceVertices.size();
    const size_t jointCount = std::max<size_t>(m_matrixPalette.size(), 1);

    const bool needsRecreate =
        (m_gpuSkinningVertexCapacity != vertexCount) ||
        (m_gpuPaletteJointCapacity != jointCount) ||
        !hasGpuSkinningResources();

    if (!needsRecreate)
        return true;

    invalidateGpuSkinningResources();

    ModuleResources* resources = app->getModuleResources();
    const size_t vertexBytes = vertexCount * sizeof(Vertex);
    const size_t paletteBytes = jointCount * sizeof(Matrix);

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        std::string outputName = "SkinnedOutputVB_" + std::to_string(i);
        ComPtr<ID3D12Resource> outputResource =
            resources->createDefaultBuffer(
                vertexBytes,
                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                outputName.c_str());

        m_gpuSkinnedVertexBuffers[i].reset(
            resources->createVertexBuffer(outputResource, vertexCount, sizeof(Vertex)));

        m_gpuPaletteModelBuffers[i] = resources->createUploadBuffer(paletteBytes);
        m_gpuPaletteNormalBuffers[i] = resources->createUploadBuffer(paletteBytes);
    }

    m_gpuSkinningVertexCapacity = vertexCount;
    m_gpuPaletteJointCapacity = jointCount;
    return true;
}

void MeshRenderer::updateGpuPaletteBuffers()
{
    if (m_matrixPalette.empty() || m_normalPalette.empty())
        return;

    if (!ensureGpuSkinningResources())
        return;

    const uint32_t frameIndex = GetCurrentSkinningFrameIndex();
    const size_t paletteBytes = m_matrixPalette.size() * sizeof(Matrix);

    {
        void* mapped = nullptr;
        CD3DX12_RANGE readRange(0, 0);
        m_gpuPaletteModelBuffers[frameIndex]->Map(0, &readRange, &mapped);
        memcpy(mapped, m_matrixPalette.data(), paletteBytes);
        m_gpuPaletteModelBuffers[frameIndex]->Unmap(0, nullptr);
    }

    {
        void* mapped = nullptr;
        CD3DX12_RANGE readRange(0, 0);
        m_gpuPaletteNormalBuffers[frameIndex]->Map(0, &readRange, &mapped);
        memcpy(mapped, m_normalPalette.data(), paletteBytes);
        m_gpuPaletteNormalBuffers[frameIndex]->Unmap(0, nullptr);
    }
}

void MeshRenderer::invalidateGpuSkinningResources()
{
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        m_gpuSkinnedVertexBuffers[i].reset();
        m_gpuPaletteModelBuffers[i].Reset();
        m_gpuPaletteNormalBuffers[i].Reset();
    }

    m_gpuSkinningVertexCapacity = 0;
    m_gpuPaletteJointCapacity = 0;
}

void MeshRenderer::cacheSourceVertices(const MeshAsset& meshAsset)
{
    const Vertex* src = static_cast<const Vertex*>(meshAsset.getVertexData());
    const uint32_t count = meshAsset.getVertexCount();

    if (!src || count == 0)
    {
        m_sourceVertices.clear();
        m_skinnedVertices.clear();
        m_skinnedVertexBuffer.reset();
        return;
    }

    m_sourceVertices.assign(src, src + count);
    m_skinnedVertices.clear();
    m_skinnedVertexBuffer.reset();

    invalidateGpuSkinningResources();
}

void MeshRenderer::rebuildCpuSkinnedVertexBuffer()
{
    if (!m_mesh || m_sourceVertices.empty() || m_matrixPalette.empty())
    {
        m_skinnedVertices.clear();
        m_skinnedVertexBuffer.reset();
        return;
    }

    m_skinnedVertices = m_sourceVertices;

    const size_t paletteSize = m_matrixPalette.size();

    for (size_t i = 0; i < m_sourceVertices.size(); ++i)
    {
        const Vertex& src = m_sourceVertices[i];
        Vertex& dst = m_skinnedVertices[i];

        Vector3 skinnedPos = Vector3::Zero;
        Vector3 skinnedNormal = Vector3::Zero;
        float totalWeight = 0.0f;

        for (int j = 0; j < 4; ++j)
        {
            const uint32_t jointIndex = src.joints[j];
            const float weight = GetWeightComponent(src.weights, j);

            if (weight <= 0.0f)
                continue;

            if (jointIndex >= paletteSize)
                continue;

            skinnedPos += Vector3::Transform(src.position, m_matrixPalette[jointIndex]) * weight;
            skinnedNormal += Vector3::TransformNormal(src.normal, m_normalPalette[jointIndex]) * weight;
            totalWeight += weight;
        }

        if (totalWeight > 0.0f)
        {
            dst.position = skinnedPos;

            if (skinnedNormal.LengthSquared() > 0.0f)
            {
                skinnedNormal.Normalize();
                dst.normal = skinnedNormal;
            }
        }
    }

    m_skinnedVertexBuffer.reset(
        app->getModuleResources()->createVertexBuffer(
            m_skinnedVertices.data(),
            m_skinnedVertices.size(),
            sizeof(Vertex))
    );
}
