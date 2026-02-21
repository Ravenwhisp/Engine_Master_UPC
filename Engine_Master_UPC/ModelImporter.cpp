#include "Globals.h"
#include "ModelImporter.h"
#include <Logger.h>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#define TINYGLTF_IMPLEMENTATION /* Only in one of the includes */
#pragma warning(push)
#pragma warning(disable : 4018) 
#pragma warning(disable : 4267) 
#include "tiny_gltf.h"
#pragma warning(pop)

#include "Application.h"
#include "AssetsModule.h"
#include <IndexBuffer.h>

static const DXGI_FORMAT INDEX_FORMATS[3] = { DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R32_UINT };


bool ModelImporter::loadExternal(const std::filesystem::path& path, tinygltf::Model& out)
{
	tinygltf::TinyGLTF gltfContext;
	std::string error, warning;

	std::string pathStr = path.string();
	const char* cpath = pathStr.c_str();

	if (!gltfContext.LoadASCIIFromFile(&out, &error, &warning, cpath))
	{
		LOG_ERROR("Failed to load model from file: %s", cpath);
		return false;
	}

	return true;
}

void ModelImporter::loadMesh(const tinygltf::Model& model, const tinygltf::Primitive& primitive, MeshAsset* mesh, const std::vector<uint32_t>& materialRemap)
{
    const uint32_t baseVertex = static_cast<uint32_t>(mesh->vertices.size());
    const uint32_t baseIndex = static_cast<uint32_t>(mesh->indices.size());

    auto itPos = primitive.attributes.find("POSITION");
    if (itPos == primitive.attributes.end()) return;

    const tinygltf::Accessor& positionAccessor = model.accessors[itPos->second];
    const uint32_t vertexCount = static_cast<uint32_t>(positionAccessor.count);

    mesh->vertices.resize(baseVertex + vertexCount);

    uint8_t* vertexData = reinterpret_cast<uint8_t*>(mesh->vertices.data() + baseVertex);

    loadAccessorData(vertexData + offsetof(Vertex, position),sizeof(Vector3), sizeof(Vertex), vertexCount, model, itPos->second);
    loadAccessorData(vertexData + offsetof(Vertex, normal), sizeof(Vector3), sizeof(Vertex), vertexCount, model, primitive.attributes, "NORMAL");
    loadAccessorData(vertexData + offsetof(Vertex, texCoord0), sizeof(Vector2), sizeof(Vertex), vertexCount,   model, primitive.attributes, "TEXCOORD_0");

    // Indices
    uint32_t indexCount = 0;

    if (primitive.indices >= 0)
    {
        const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
		if (indexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT || indexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT || indexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE)
		{
            uint32_t componentSize = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType);

            uint32_t indexCount = static_cast<uint32_t>(indexAccessor.count);
            uint32_t byteCount = indexCount * componentSize;

            uint32_t baseOffset = static_cast<uint32_t>(mesh->indices.size());

            mesh->indices.resize(baseOffset + byteCount);

            loadAccessorData( mesh->indices.data() + baseOffset, componentSize,  componentSize, indexCount,  model, primitive.indices );

			mesh->indexFormat = INDEX_FORMATS[componentSize >> 1];
		}
    }
    else
    {
        indexCount = vertexCount;
        mesh->indices.resize(baseIndex + indexCount);

        for (uint32_t i = 0; i < vertexCount; ++i)
        {
            mesh->indices[baseIndex + i] = baseVertex + i;
        }
    }


    Submesh submesh{};
    submesh.indexStart = baseIndex;
    submesh.indexCount = indexCount;
	if (primitive.material >= 0 && primitive.material < materialRemap.size())
	{
		submesh.materialId = materialRemap[primitive.material];
	}
	else
	{
		submesh.materialId = 0;
	}

    mesh->submeshes.push_back(submesh);

    // Bounds
    if (positionAccessor.minValues.size() == 3 && positionAccessor.maxValues.size() == 3)
    {
        Vector3 min(float(positionAccessor.minValues[0]), float(positionAccessor.minValues[1]), float(positionAccessor.minValues[2]));
        Vector3 max(float(positionAccessor.maxValues[0]), float(positionAccessor.maxValues[1]), float(positionAccessor.maxValues[2]));

        mesh->boundsCenter = (min + max) * 0.5f;
        mesh->boundsExtents = (max - min) * 0.5f;
    }
}


uint32_t loadTextureFromGLTF(const tinygltf::Model& model,int gltfTextureIndex)
{
	if (gltfTextureIndex < 0 || gltfTextureIndex >= static_cast<int>(model.textures.size())) return 0;
	const tinygltf::Texture& texture = model.textures[gltfTextureIndex];

	if (texture.source < 0 || texture.source >= static_cast<int>(model.images.size())) return 0;
	const tinygltf::Image& image = model.images[texture.source];

	if (image.uri.empty()) return 0;
	std::filesystem::path texturePath = image.uri;

	return app->getAssetModule()->import(texturePath.string().c_str());
}

void ModelImporter::loadMaterial(const tinygltf::Model& model, const tinygltf::Material& material, MaterialAsset* materialAsset)
{

	tinygltf::PbrMetallicRoughness pbrMetallicRoughness = material.pbrMetallicRoughness;
	materialAsset->baseColour = Color(float(pbrMetallicRoughness.baseColorFactor[0]), float(pbrMetallicRoughness.baseColorFactor[1]), float(pbrMetallicRoughness.baseColorFactor[2]), float(pbrMetallicRoughness.baseColorFactor[3]));

	materialAsset->metallicFactor = float(pbrMetallicRoughness.metallicFactor);

	materialAsset->baseMap =				loadTextureFromGLTF(model, pbrMetallicRoughness.baseColorTexture.index);
	materialAsset->metallicRoughnessMap =	loadTextureFromGLTF(model, pbrMetallicRoughness.metallicRoughnessTexture.index);
	materialAsset->normalMap =				loadTextureFromGLTF(model, material.normalTexture.index);
	materialAsset->occlusionMap =			loadTextureFromGLTF(model, material.occlusionTexture.index);
	materialAsset->emissiveMap =			loadTextureFromGLTF(model, material.emissiveTexture.index);
	materialAsset->isEmissive = materialAsset->emissiveMap != 0;
	materialAsset->emissiveMap =			loadTextureFromGLTF(model, material.emissiveTexture.index);
}


void ModelImporter::importTyped(const tinygltf::Model& source, ModelAsset* model)
{
	//This part is to translate the materialId from each primitive to the assetId from the materials loaded by order of importing
	std::vector<uint32_t> materialIndexToAssetId;
	materialIndexToAssetId.reserve(source.materials.size());

    for (tinygltf::Material material : source.materials) 
	{		
		uint32_t materialId = app->getAssetModule()->generateNewUID();
		MaterialAsset myMaterial(materialId);

		loadMaterial(source, material, &myMaterial);

		model->materials.push_back(myMaterial);
		materialIndexToAssetId.push_back(materialId);
	}

	for (tinygltf::Mesh mesh : source.meshes) 
	{
		MeshAsset myMesh(app->getAssetModule()->generateNewUID());
		for (tinygltf::Primitive primitive : mesh.primitives) 
		{
			loadMesh(source, primitive, &myMesh, materialIndexToAssetId);
		}
		model->meshes.push_back(myMesh);
	}
}

uint64_t ModelImporter::saveTyped(const ModelAsset* model, uint8_t** buffer)
{
	return 0;
}

void ModelImporter::loadTyped(const uint8_t* buffer, ModelAsset* model)
{
}
