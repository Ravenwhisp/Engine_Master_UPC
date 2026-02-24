#pragma once
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#include "tiny_gltf.h"

bool loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model, int accesorIndex);

bool loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model,
	const std::map<std::string, int>& attributes, const char* accesorName);

