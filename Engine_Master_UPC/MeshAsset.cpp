#include "Globals.h"
#include "MeshAsset.h"
#include "IArchive.h"

#include "IndexBuffer.h"

#include <imgui.h>

std::vector<Vector3> MeshAsset::getVerticesPositions() const
{
    std::vector<Vector3> positions;
    positions.reserve(vertices.size());

    for (const Vertex& vertex : vertices)
    {
        positions.push_back(vertex.position);
    }

    return positions;
}

uint32_t MeshAsset::getIndexCount() const
{
    return static_cast<uint32_t>(indices.size()) / getSizeByFormat(indexFormat);
}

void MeshAsset::drawUI()
{
    ImGui::Text("Mesh Asset");

    ImGui::Separator();

    ImGui::Text("UID: %llu", static_cast<unsigned long long>(getUID()));

    ImGui::SeparatorText("Geometry");

    ImGui::Text("Vertices: %u", getVertexCount());
    ImGui::Text("Vertex stride: %u bytes", getVertexStride());

    ImGui::Text("Indices: %u", getIndexCount());
    ImGui::Text("Index buffer size: %u bytes", getIndexBufferSize());
    ImGui::Text("Index format: %d", static_cast<int>(indexFormat));

    ImGui::Text("Submeshes: %zu", submeshes.size());

    if (ImGui::TreeNode("Submesh list"))
    {
        for (size_t i = 0; i < submeshes.size(); ++i)
        {
            const Submesh& submesh = submeshes[i];

            ImGui::PushID(static_cast<int>(i));
            ImGui::Text("Submesh %zu", i);
            ImGui::Text("Index start: %u", submesh.indexStart);
            ImGui::Text("Index count: %u", submesh.indexCount);
            ImGui::Separator();
            ImGui::PopID();
        }

        ImGui::TreePop();
    }

    ImGui::SeparatorText("Bounds");

    ImGui::Text(
        "Center: %.3f, %.3f, %.3f",
        boundsCenter.x,
        boundsCenter.y,
        boundsCenter.z
    );

    ImGui::Text(
        "Extents: %.3f, %.3f, %.3f",
        boundsExtents.x,
        boundsExtents.y,
        boundsExtents.z
    );
}

void MeshAsset::serialize(IArchive& archive)
{
    uint32_t vertexCount = static_cast<uint32_t>(vertices.size());
    archive.serialize(vertexCount);
    if (archive.mode() == ArchiveMode::Input)
        vertices.resize(vertexCount);

    for (auto& v : vertices)
    {
        archive.serializeRaw(&v.position, sizeof(Vector3));
        archive.serializeRaw(&v.texCoord0, sizeof(Vector2));
        archive.serializeRaw(&v.normal, sizeof(Vector3));
        archive.serializeRaw(&v.tangent, sizeof(Vector3));
        archive.serializeRaw(v.joints, sizeof(uint16_t) * 4);
        archive.serializeRaw(&v.weights, sizeof(Vector4));
    }

    uint32_t indexByteCount = static_cast<uint32_t>(indices.size());
    archive.serialize(indexByteCount);
    uint32_t idxFormat = static_cast<uint32_t>(indexFormat);
    archive.serialize(idxFormat);
    indexFormat = static_cast<DXGI_FORMAT>(idxFormat);
    if (archive.mode() == ArchiveMode::Input)
        indices.resize(indexByteCount);
    archive.serializeRaw(indices.data(), indexByteCount);

    uint32_t submeshCount = static_cast<uint32_t>(submeshes.size());
    archive.serialize(submeshCount);
    if (archive.mode() == ArchiveMode::Input)
        submeshes.resize(submeshCount);
    for (auto& sm : submeshes)
    {
        archive.serialize(sm.indexStart);
        archive.serialize(sm.indexCount);
    }

    archive.serializeRaw(&boundsCenter, sizeof(Vector3));
    archive.serializeRaw(&boundsExtents, sizeof(Vector3));
}