#include "Globals.h"
#include "ImporterGltf.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#define TINYGLTF_IMPLEMENTATION /* Only in one of the includes */
#pragma warning(push)
#pragma warning(disable : 4018) 
#pragma warning(disable : 4267) 
#include "tiny_gltf.h"
#pragma warning(pop)

#include "ModelAsset.h"
#include "MeshAsset.h"
#include "MaterialAsset.h"

#include "Application.h"
#include "ModuleAssets.h"

static const DXGI_FORMAT INDEX_FORMATS[3] = { DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R32_UINT };


bool ImporterGltf::canImport(const std::filesystem::path& path) const
{
	auto ext = path.extension().string();
	return ext == GLTF_EXTENSION;
}

Asset* ImporterGltf::createAssetInstance(const MD5Hash& uid) const
{
	return new ModelAsset(uid);
}

bool ImporterGltf::loadExternal(const std::filesystem::path& path, tinygltf::Model& out)
{
	tinygltf::TinyGLTF gltfContext;
	std::string error, warning;

	std::string pathStr = path.string();
	const char* cpath = pathStr.c_str();

	if (!gltfContext.LoadASCIIFromFile(&out, &error, &warning, cpath))
	{
		DEBUG_ERROR("Failed to load model from file: %s", cpath);
		return false;
	}

	// In case of the gltf file we need to store temporarly the path since when trying to load the images, they only have the relative path
	m_currentFilePath = &path;
	return true;
}

void ImporterGltf::importTyped(const tinygltf::Model& source, ModelAsset* model)
{
}

uint64_t ImporterGltf::saveTyped(const ModelAsset* model, uint8_t** outBuffer)
{
    return 0;
}

void ImporterGltf::loadTyped(const uint8_t* buffer, ModelAsset* model)
{
}

void ImporterGltf::loadMesh(const tinygltf::Model& model, const tinygltf::Primitive& primitive, MeshAsset* mesh, const std::vector<MD5Hash>& materialRemap)
{
    const uint32_t baseVertex = static_cast<uint32_t>(mesh->vertices.size());
    const uint32_t baseIndex = static_cast<uint32_t>(mesh->indices.size());

    auto itPos = primitive.attributes.find("POSITION");
    if (itPos == primitive.attributes.end()) return;

    const tinygltf::Accessor& positionAccessor = model.accessors[itPos->second];
    const uint32_t vertexCount = static_cast<uint32_t>(positionAccessor.count);

    mesh->vertices.resize(baseVertex + vertexCount);

    uint8_t* vertexData = reinterpret_cast<uint8_t*>(mesh->vertices.data() + baseVertex);

    loadAccessorData(vertexData + offsetof(Vertex, position), sizeof(Vector3), sizeof(Vertex), vertexCount, model, itPos->second);
    loadAccessorData(vertexData + offsetof(Vertex, normal), sizeof(Vector3), sizeof(Vertex), vertexCount, model, primitive.attributes, "NORMAL");
    loadAccessorData(vertexData + offsetof(Vertex, texCoord0), sizeof(Vector2), sizeof(Vertex), vertexCount, model, primitive.attributes, "TEXCOORD_0");

    // Indices
    uint32_t indexCount = 0;
    uint32_t componentSize = 0;
    if (primitive.indices >= 0)
    {
        const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
        if (indexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT || indexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT || indexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE)
        {
            componentSize = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType);

            indexCount = static_cast<uint32_t>(indexAccessor.count);
            uint32_t byteCount = indexCount * componentSize;

            uint32_t baseOffset = static_cast<uint32_t>(mesh->indices.size());

            mesh->indices.resize(baseOffset + byteCount);

            loadAccessorData(mesh->indices.data() + baseOffset, componentSize, componentSize, indexCount, model, primitive.indices);

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
    submesh.indexStart = baseIndex / componentSize;
    submesh.indexCount = indexCount;
    if (primitive.material >= 0 && primitive.material < materialRemap.size())
    {
        submesh.materialId = materialRemap[primitive.material];
    }
    else
    {
        submesh.materialId = INVALID_ASSET_ID;
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


MD5Hash loadTextureFromGLTF(const tinygltf::Model& model, int gltfTextureIndex, const std::filesystem::path* modelPath)
{
    if (gltfTextureIndex < 0 || gltfTextureIndex >= static_cast<int>(model.textures.size()))
    {
        return INVALID_ASSET_ID;
    }

    const tinygltf::Texture& texture = model.textures[gltfTextureIndex];

    if (texture.source < 0 || texture.source >= static_cast<int>(model.images.size()))
    {
        return INVALID_ASSET_ID;
    }

    const tinygltf::Image& image = model.images[texture.source];

    if (image.uri.empty())
    {
        return INVALID_ASSET_ID;
    }

    std::filesystem::path resolvedPath = modelPath->parent_path() / image.uri;

    MD5Hash uid = app->getModuleAssets()->findUID(resolvedPath);
    if (uid == INVALID_ASSET_ID)
    {
        app->getModuleAssets()->importAsset(resolvedPath, uid);
        return uid;
    }

    return uid;
}

void ImporterGltf::loadMaterial(const tinygltf::Model& model, const tinygltf::Material& material, MaterialAsset* materialAsset)
{

    tinygltf::PbrMetallicRoughness pbrMetallicRoughness = material.pbrMetallicRoughness;
    materialAsset->baseColour = Color(float(pbrMetallicRoughness.baseColorFactor[0]), float(pbrMetallicRoughness.baseColorFactor[1]), float(pbrMetallicRoughness.baseColorFactor[2]), float(pbrMetallicRoughness.baseColorFactor[3]));

    materialAsset->metallicFactor = float(pbrMetallicRoughness.metallicFactor);

    materialAsset->baseMap = loadTextureFromGLTF(model, pbrMetallicRoughness.baseColorTexture.index, m_currentFilePath);
    materialAsset->metallicRoughnessMap = loadTextureFromGLTF(model, pbrMetallicRoughness.metallicRoughnessTexture.index, m_currentFilePath);
    materialAsset->normalMap = loadTextureFromGLTF(model, material.normalTexture.index, m_currentFilePath);
    materialAsset->occlusionMap = loadTextureFromGLTF(model, material.occlusionTexture.index, m_currentFilePath);
    materialAsset->emissiveMap = loadTextureFromGLTF(model, material.emissiveTexture.index, m_currentFilePath);
    materialAsset->isEmissive = materialAsset->emissiveMap != INVALID_ASSET_ID;
    materialAsset->emissiveMap = loadTextureFromGLTF(model, material.emissiveTexture.index, m_currentFilePath);
}
