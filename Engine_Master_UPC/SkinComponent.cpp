#include "Globals.h"
#include "SkinComponent.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "GameObject.h"
#include "Transform.h"
#include "SkinAsset.h"
#include "MeshRenderer.h"
#include "MeshAsset.h"
#include "ModuleResources.h"
#include "VertexBuffer.h"
#include "ModuleD3D12.h"

#include <imgui.h>
#include <algorithm>


namespace
{
    GameObject* FindHierarchyRoot(GameObject* go)
    {
        if (!go)
            return nullptr;

        GameObject* current = go;

        while (current)
        {
            Transform* transform = current->GetTransform();
            if (!transform)
                break;

            Transform* parentTransform = transform->getRoot();
            if (!parentTransform)
                break;

            current = parentTransform->getOwner();
        }

        return current;
    }

    GameObject* FindByNameRecursive(GameObject* go, const std::string& name)
    {
        if (!go)
            return nullptr;

        if (go->GetName() == name)
            return go;

        Transform* transform = go->GetTransform();
        if (!transform)
            return nullptr;

        for (GameObject* child : transform->getAllChildren())
        {
            if (GameObject* found = FindByNameRecursive(child, name))
                return found;
        }

        return nullptr;
    }

    Matrix BuildNormalMatrixFromSkinMatrix(const Matrix& skinMatrix)
    {
        Matrix normal = skinMatrix;
        normal.Translation(Vector3::Zero);
        normal = normal.Invert();
        normal = normal.Transpose();
        return normal;
    }

    float GetWeightComponent(const Vector4& w, int index)
    {
        switch (index)
        {
        case 0: return w.x;
        case 1: return w.y;
        case 2: return w.z;
        default: return w.w;
        }
    }

    uint32_t GetCurrentSkinningFrameIndex()
    {
        return app->getModuleD3D12()->getCurrentFrameIndex();
    }
}

SkinComponent::SkinComponent(UID id, GameObject* owner)
    : Component(id, ComponentType::SKIN, owner)
{
}

std::unique_ptr<Component> SkinComponent::clone(GameObject* newOwner) const
{
    auto cloned = std::make_unique<SkinComponent>(m_uuid, newOwner);

    cloned->m_skinAsset = m_skinAsset;

    cloned->m_skin.reset();
    cloned->m_jointTransforms.clear();
    cloned->m_skinBindingsResolved = false;
    cloned->m_matrixPalette.clear();
    cloned->m_normalPalette.clear();
    cloned->m_sourceVertices.clear();
    cloned->m_skinnedVertices.clear();
    cloned->m_skinnedVertexBuffer.reset();
    cloned->m_enableCpuSkinningFallback = m_enableCpuSkinningFallback;
    cloned->m_cachedMeshAsset = INVALID_ASSET_ID;
    cloned->invalidateGpuSkinningResources();
    cloned->setActive(isActive());

    return cloned;
}

bool SkinComponent::init()
{
    return true;
}

void SkinComponent::lateUpdate()
{
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

    ensureSourceVerticesCached();

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

bool SkinComponent::cleanUp()
{
    invalidateSkinningRuntime();
    return true;
}

void SkinComponent::setSkinReference(const MD5Hash& skinUID)
{
    if (m_skinAsset == skinUID)
        return;

    m_skinAsset = skinUID;
    invalidateSkinningRuntime();
}

void SkinComponent::drawUi()
{
    ImGui::Text("Skin Asset: %s", m_skinAsset != INVALID_ASSET_ID ? m_skinAsset.c_str() : "None");
    ImGui::Text("Skin Loaded: %s", m_skin ? "Yes" : "No");
    ImGui::Text("Resolved Joints: %d", static_cast<int>(m_jointTransforms.size()));
    ImGui::Text("Skin Bindings Resolved: %s", m_skinBindingsResolved ? "Yes" : "No");
    ImGui::Text("Matrix Palette Size: %d", static_cast<int>(m_matrixPalette.size()));
    ImGui::Text("Normal Palette Size: %d", static_cast<int>(m_normalPalette.size()));
    ImGui::Text("Source Vertices: %d", static_cast<int>(m_sourceVertices.size()));
    ImGui::Text("CPU Skinned VB: %s", m_skinnedVertexBuffer ? "Yes" : "No");
    ImGui::Checkbox("Enable CPU Skinning Fallback", &m_enableCpuSkinningFallback);
    ImGui::Text("GPU Skinning Vertex Capacity: %d", static_cast<int>(m_gpuSkinningVertexCapacity));
    ImGui::Text("GPU Palette Joint Capacity: %d", static_cast<int>(m_gpuPaletteJointCapacity));
    ImGui::Text("GPU Skinning Resources: %s", hasGpuSkinningResources() ? "Yes" : "No");
}

rapidjson::Value SkinComponent::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", static_cast<int>(getType()), domTree.GetAllocator());
    componentInfo.AddMember("Active", isActive(), domTree.GetAllocator());
    componentInfo.AddMember("SkinAssetId", rapidjson::Value(m_skinAsset.c_str(), domTree.GetAllocator()), domTree.GetAllocator());

    return componentInfo;
}

bool SkinComponent::deserializeJSON(const rapidjson::Value& componentValue)
{
    if (componentValue.HasMember("Active") && componentValue["Active"].IsBool())
    {
        setActive(componentValue["Active"].GetBool());
    }

    if (componentValue.HasMember("SkinAssetId") && componentValue["SkinAssetId"].IsString())
    {
        m_skinAsset = componentValue["SkinAssetId"].GetString();
    }
    else
    {
        m_skinAsset = INVALID_ASSET_ID;
    }

    invalidateSkinningRuntime();

    return true;

}

bool SkinComponent::ensureSkinLoaded()
{
    if (m_skinAsset == INVALID_ASSET_ID)
        return false;

    if (m_skin)
        return true;

    ModuleAssets* moduleAssets = app ? app->getModuleAssets() : nullptr;
    if (!moduleAssets)
        return false;

    std::shared_ptr<SkinAsset> skinAsset = moduleAssets->load<SkinAsset>(m_skinAsset);
    if (!skinAsset)
    {
        DEBUG_WARN("[SkinComponent] Could not load SkinAsset '%s'.", m_skinAsset.c_str());
        return false;
    }

    m_skin = skinAsset;
    m_skinBindingsResolved = false;

    return true;
}

bool SkinComponent::resolveSkinBindings()
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
            DEBUG_WARN("[SkinComponent] Joint '%s' not found while resolving skin '%s'.",
                joint.nodeName.c_str(),
                m_skin->getName().c_str());

            m_jointTransforms.clear();
            m_skinBindingsResolved = false;
            return false;
        }

        m_jointTransforms.push_back(jointGo->GetTransform());
    }

    m_skinBindingsResolved = true;
    return true;
}

void SkinComponent::invalidateSkinningRuntime()
{
    m_skin.reset();
    m_jointTransforms.clear();
    m_matrixPalette.clear();
    m_normalPalette.clear();
    m_sourceVertices.clear();
    m_skinnedVertices.clear();
    m_skinnedVertexBuffer.reset();
    m_cachedMeshAsset = INVALID_ASSET_ID;
    invalidateGpuSkinningResources();
    m_skinBindingsResolved = false;
}

void SkinComponent::rebuildMatrixPalette()
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

bool SkinComponent::ensureSourceVerticesCached()
{
    MeshRenderer* meshRenderer =
        m_owner ? m_owner->GetComponentAs<MeshRenderer>(ComponentType::MODEL) : nullptr;

    if (!meshRenderer)
    {
        m_sourceVertices.clear();
        m_skinnedVertices.clear();
        m_skinnedVertexBuffer.reset();
        m_cachedMeshAsset = INVALID_ASSET_ID;
        return false;
    }

    const MD5Hash& meshUID = meshRenderer->getMeshReference();

    if (meshUID == INVALID_ASSET_ID)
    {
        m_sourceVertices.clear();
        m_skinnedVertices.clear();
        m_skinnedVertexBuffer.reset();
        m_cachedMeshAsset = INVALID_ASSET_ID;
        return false;
    }

    if (m_cachedMeshAsset == meshUID && !m_sourceVertices.empty())
        return true;

    ModuleAssets* moduleAssets = app ? app->getModuleAssets() : nullptr;
    if (!moduleAssets)
        return false;

    std::shared_ptr<MeshAsset> meshAsset = moduleAssets->load<MeshAsset>(meshUID);
    if (!meshAsset)
    {
        DEBUG_WARN("[SkinComponent] Could not load MeshAsset '%s'.", meshUID.c_str());

        m_sourceVertices.clear();
        m_skinnedVertices.clear();
        m_skinnedVertexBuffer.reset();
        m_cachedMeshAsset = INVALID_ASSET_ID;
        return false;
    }

    cacheSourceVertices(*meshAsset);
    m_cachedMeshAsset = meshUID;
    return true;
}

void SkinComponent::cacheSourceVertices(const MeshAsset& meshAsset)
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

void SkinComponent::rebuildCpuSkinnedVertexBuffer()
{
    if (m_sourceVertices.empty() || m_matrixPalette.empty())
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
            skinnedPos /= totalWeight;
            dst.position = skinnedPos;

            if (skinnedNormal.LengthSquared() > 0.0f)
            {
                skinnedNormal.Normalize();
                dst.normal = skinnedNormal;
            }
        }
    }

    ModuleResources* moduleResources = app ? app->getModuleResources() : nullptr;
    if (!moduleResources)
    {
        m_skinnedVertexBuffer.reset();
        return;
    }

    m_skinnedVertexBuffer.reset(
        moduleResources->createVertexBuffer(
            m_skinnedVertices.data(),
            m_skinnedVertices.size(),
            sizeof(Vertex))
    );
}

const VertexBuffer* SkinComponent::getCurrentGpuSkinnedVertexBuffer() const
{
    const uint32_t frameIndex = GetCurrentSkinningFrameIndex();
    return m_gpuSkinnedVertexBuffers[frameIndex].get();
}

ID3D12Resource* SkinComponent::getCurrentGpuSkinnedOutputResource() const
{
    const uint32_t frameIndex = GetCurrentSkinningFrameIndex();
    return m_gpuSkinnedVertexBuffers[frameIndex]
        ? m_gpuSkinnedVertexBuffers[frameIndex]->getD3D12Resource().Get()
        : nullptr;
}

ID3D12Resource* SkinComponent::getCurrentGpuPaletteModelResource() const
{
    const uint32_t frameIndex = GetCurrentSkinningFrameIndex();
    return m_gpuPaletteModelBuffers[frameIndex].Get();
}

ID3D12Resource* SkinComponent::getCurrentGpuPaletteNormalResource() const
{
    const uint32_t frameIndex = GetCurrentSkinningFrameIndex();
    return m_gpuPaletteNormalBuffers[frameIndex].Get();
}

bool SkinComponent::hasGpuSkinningResources() const
{
    const uint32_t frameIndex = GetCurrentSkinningFrameIndex();

    return m_gpuSkinnedVertexBuffers[frameIndex] != nullptr
        && m_gpuPaletteModelBuffers[frameIndex] != nullptr
        && m_gpuPaletteNormalBuffers[frameIndex] != nullptr;
}

bool SkinComponent::ensureGpuSkinningResources()
{
    if (m_sourceVertices.empty())
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

    ModuleResources* resources = app ? app->getModuleResources() : nullptr;
    if (!resources)
        return false;

    const size_t vertexBytes = vertexCount * sizeof(Vertex);
    const size_t paletteBytes = jointCount * sizeof(Matrix);

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        std::string outputName = "SkinComponent_SkinnedOutputVB_" + std::to_string(i);

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

void SkinComponent::updateGpuPaletteBuffers()
{
    if (m_matrixPalette.empty() || m_normalPalette.empty())
        return;

    if (!ensureSourceVerticesCached())
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

void SkinComponent::invalidateGpuSkinningResources()
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