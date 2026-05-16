#pragma once
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#include "tiny_gltf.h"

#include "Math.h"

#include <cstdint>
#include <string>
#include <vector>

bool loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model, int accesorIndex);

bool loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model,
	const std::map<std::string, int>& attributes, const char* accesorName);

template<typename T>
bool loadAccessorTyped(const tinygltf::Model& model, int accessorIdx, std::vector<T>& out)
{
    if (accessorIdx < 0 || accessorIdx >= static_cast<int>(model.accessors.size()))
        return false;

    const auto& acc = model.accessors[accessorIdx];
    out.resize(acc.count);
    return loadAccessorData(reinterpret_cast<uint8_t*>(out.data()),  sizeof(T), sizeof(T), acc.count, model, accessorIdx);
}

bool loadJointIndices4(const tinygltf::Model& model, int accessorIdx, uint16_t* dst, uint32_t count, uint32_t dstStride);


void collapseToTop4Influences( const uint16_t joints0[4], const Vector4& weights0, const uint16_t joints1[4], const Vector4& weights1, uint16_t outJoints[4], Vector4& outWeights);


std::string resolveNodeName(const tinygltf::Model& model, int nodeIdx);