#include "Globals.h"
#include "ModelImporter.h"
#include <Logger.h>


#include "Application.h"
#include "FileSystemModule.h"

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


void ModelImporter::loadMesh(const tinygltf::Model& model, const tinygltf::Primitive& primitive, MeshAsset* mesh)
{
    assert(mesh);

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
        const tinygltf::Accessor& idexAccessor = model.accessors[primitive.indices];
		if (idexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT || idexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT || idexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE)
		{
			uint32_t indexElementSize = tinygltf::GetComponentSizeInBytes(idexAccessor.componentType);
			uint32_t numIndices = uint32_t(idexAccessor.count);
			uint8_t* indices = new uint8_t[numIndices * indexElementSize];
			loadAccessorData(indices, indexElementSize, indexElementSize, numIndices, model, primitive.indices);

			mesh->indices.resize(baseIndex + numIndices);
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
    submesh.materialId = (primitive.material >= 0)  ? primitive.material : 0;

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

	Asset * asset = app->getFileSystemModule()->import(texturePath.string().c_str());
	if (!asset) return 0;

	return asset->getId();
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
    for (tinygltf::Material material : source.materials) 
	{		
		MaterialAsset myMaterial(rand());
		loadMaterial(source, material, &myMaterial);
		model->materials.push_back(myMaterial);
	}

	for (tinygltf::Mesh mesh : source.meshes) 
	{
		MeshAsset myMesh(rand());
		for (tinygltf::Primitive primitive : mesh.primitives) 
		{
			loadMesh(source, primitive, &myMesh);
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
