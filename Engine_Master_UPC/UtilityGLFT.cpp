#include "Globals.h"
#include "UtilityGLFT.h"

#include <algorithm>
#include <array>

namespace
{
    struct JointInfluence
    {
        uint16_t joint = 0;
        float    weight = 0.0f;
    };

    float getVec4Component(const Vector4& v, int index)
    {
        switch (index)
        {
        case 0: return v.x;
        case 1: return v.y;
        case 2: return v.z;
        default: return v.w;
        }
    }

    void setVec4Component(Vector4& v, int index, float value)
    {
        switch (index)
        {
        case 0: v.x = value; break;
        case 1: v.y = value; break;
        case 2: v.z = value; break;
        default: v.w = value; break;
        }
    }
} 


bool loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount,
    const tinygltf::Model& model, int accesorIndex)
{
    const tinygltf::Accessor& accessor = model.accessors[accesorIndex];
    const size_t defaultStride =
        tinygltf::GetComponentSizeInBytes(accessor.componentType) *
        tinygltf::GetNumComponentsInType(accessor.type);

    if (elemCount == accessor.count && defaultStride == elemSize)
    {
        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
        const uint8_t* bufferData = reinterpret_cast<const uint8_t*>(
            &(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
        const size_t bufferStride = view.byteStride == 0 ? defaultStride : view.byteStride;

        for (uint32_t i = 0; i < elemCount; ++i)
        {
            memcpy(data, bufferData, elemSize);
            data += stride;
            bufferData += bufferStride;
        }

        return true;
    }

    return false;
}

bool loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount,
    const tinygltf::Model& model,
    const std::map<std::string, int>& attributes, const char* accesorName)
{
    const auto it = attributes.find(accesorName);
    if (it != attributes.end())
        return loadAccessorData(data, elemSize, stride, elemCount, model, it->second);

    return false;
}


bool loadJointIndices4(const tinygltf::Model& model, int accessorIdx,
    uint16_t* dst, uint32_t count, uint32_t dstStride)
{
    if (accessorIdx < 0 || accessorIdx >= static_cast<int>(model.accessors.size()))
        return false;

    const tinygltf::Accessor& acc = model.accessors[accessorIdx];
    if (acc.type != TINYGLTF_TYPE_VEC4)                              return false;
    if (acc.bufferView < 0 || acc.bufferView >= static_cast<int>(model.bufferViews.size())) return false;

    const tinygltf::BufferView& view = model.bufferViews[acc.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[view.buffer];

    const uint8_t* src = buffer.data.data() + view.byteOffset + acc.byteOffset;
    const size_t   srcStride = acc.ByteStride(view);
    if (srcStride == 0) return false;

    for (uint32_t i = 0; i < count; ++i)
    {
        uint16_t* out = reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(dst) + i * dstStride);
        const uint8_t* in = src + i * srcStride;

        switch (acc.componentType)
        {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        {
            const auto* v = reinterpret_cast<const uint8_t*>(in);
            out[0] = v[0]; out[1] = v[1]; out[2] = v[2]; out[3] = v[3];
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        {
            const auto* v = reinterpret_cast<const uint16_t*>(in);
            out[0] = v[0]; out[1] = v[1]; out[2] = v[2]; out[3] = v[3];
            break;
        }
        default:
            return false;
        }
    }

    return true;
}


void collapseToTop4Influences(
    const uint16_t joints0[4], const Vector4& weights0,
    const uint16_t joints1[4], const Vector4& weights1,
    uint16_t outJoints[4], Vector4& outWeights)
{
    std::array<JointInfluence, 8> influences{};
    int count = 0;

    for (int i = 0; i < 4; ++i)
    {
        const float w = getVec4Component(weights0, i);
        if (w > 0.0f) influences[count++] = { joints0[i], w };
    }

    for (int i = 0; i < 4; ++i)
    {
        const float w = getVec4Component(weights1, i);
        if (w > 0.0f) influences[count++] = { joints1[i], w };
    }

    std::sort(influences.begin(), influences.begin() + count,
        [](const JointInfluence& a, const JointInfluence& b) { return a.weight > b.weight; });

    for (int i = 0; i < 4; ++i) outJoints[i] = 0;
    outWeights = Vector4::Zero;

    const int   keptCount = std::min(count, 4);
    float       totalWeight = 0.0f;

    for (int i = 0; i < keptCount; ++i)
    {
        outJoints[i] = influences[i].joint;
        setVec4Component(outWeights, i, influences[i].weight);
        totalWeight += influences[i].weight;
    }

    if (totalWeight > 0.0f)
        outWeights /= totalWeight;
}
std::string resolveNodeName(const tinygltf::Model& model, int nodeIdx)
{
    if (nodeIdx < 0 || nodeIdx >= static_cast<int>(model.nodes.size())) return "";
    const std::string& n = model.nodes[nodeIdx].name;
    return n.empty() ? ("Node_" + std::to_string(nodeIdx)) : n;
}