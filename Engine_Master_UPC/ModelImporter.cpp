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
#include <UID.h>

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

    // In case of the gltf file we need to store temporarly the path since when trying to load the images, they only have the relative path
    m_currentFilePath = &path;
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

            indexCount = static_cast<uint32_t>(indexAccessor.count);
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


UID loadTextureFromGLTF(const tinygltf::Model& model,int gltfTextureIndex, const std::filesystem::path* modelPath)
{
	if (gltfTextureIndex < 0 || gltfTextureIndex >= static_cast<int>(model.textures.size())) return 0;
	const tinygltf::Texture& texture = model.textures[gltfTextureIndex];

	if (texture.source < 0 || texture.source >= static_cast<int>(model.images.size())) return 0;
	const tinygltf::Image& image = model.images[texture.source];

	if (image.uri.empty()) return 0;
    std::filesystem::path resolvedPath = modelPath->parent_path() / image.uri;

    UID uid = app->getAssetModule()->find(resolvedPath.string().c_str());
    if (uid == INVALID_ASSET_ID)
    {
        return app->getAssetModule()->import(resolvedPath.string().c_str());
    }

	return uid;
}

void ModelImporter::loadMaterial(const tinygltf::Model& model, const tinygltf::Material& material, MaterialAsset* materialAsset)
{

	tinygltf::PbrMetallicRoughness pbrMetallicRoughness = material.pbrMetallicRoughness;
	materialAsset->baseColour = Color(float(pbrMetallicRoughness.baseColorFactor[0]), float(pbrMetallicRoughness.baseColorFactor[1]), float(pbrMetallicRoughness.baseColorFactor[2]), float(pbrMetallicRoughness.baseColorFactor[3]));

	materialAsset->metallicFactor = float(pbrMetallicRoughness.metallicFactor);

	materialAsset->baseMap =				loadTextureFromGLTF(model, pbrMetallicRoughness.baseColorTexture.index, m_currentFilePath);
	materialAsset->metallicRoughnessMap =	loadTextureFromGLTF(model, pbrMetallicRoughness.metallicRoughnessTexture.index, m_currentFilePath);
	materialAsset->normalMap =				loadTextureFromGLTF(model, material.normalTexture.index, m_currentFilePath);
	materialAsset->occlusionMap =			loadTextureFromGLTF(model, material.occlusionTexture.index, m_currentFilePath);
	materialAsset->emissiveMap =			loadTextureFromGLTF(model, material.emissiveTexture.index, m_currentFilePath);
	materialAsset->isEmissive = materialAsset->emissiveMap != 0;
	materialAsset->emissiveMap =			loadTextureFromGLTF(model, material.emissiveTexture.index, m_currentFilePath);
}


void ModelImporter::importTyped(const tinygltf::Model& source, ModelAsset* model)
{
	//This part is to translate the materialId from each primitive to the assetId from the materials loaded by order of importing
	std::vector<uint32_t> materialIndexToAssetId;
	materialIndexToAssetId.reserve(source.materials.size());

    for (tinygltf::Material material : source.materials) 
	{		
        uint32_t materialId = GenerateUID();
		MaterialAsset myMaterial(materialId);

		loadMaterial(source, material, &myMaterial);

		model->materials.push_back(myMaterial);
		materialIndexToAssetId.push_back(materialId);
	}

	for (tinygltf::Mesh mesh : source.meshes) 
	{
		MeshAsset myMesh(GenerateUID());
		for (tinygltf::Primitive primitive : mesh.primitives) 
		{
			loadMesh(source, primitive, &myMesh, materialIndexToAssetId);
		}
		model->meshes.push_back(myMesh);
	}
}

uint64_t ModelImporter::saveTyped(const ModelAsset* source, uint8_t** outBuffer)
{
    uint64_t size = sizeof(uint64_t); // uid

    size += sizeof(uint32_t); // materialCount
    for (const auto& mat : source->materials)
    {
        uint8_t* tmp = nullptr;
        uint64_t matSize = saveMaterial(&mat, &tmp);
        size += sizeof(uint32_t) + matSize;
        delete[] tmp;
    }

    size += sizeof(uint32_t); // meshCount
    for (const auto& mesh : source->meshes)
    {
        uint8_t* tmp = nullptr;
        uint64_t meshSize = saveMesh(&mesh, &tmp);
        size += sizeof(uint32_t) + meshSize;
        delete[] tmp;
    }

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter writer(buffer);

    writer.u64(source->m_uid);

    writer.u32(static_cast<uint32_t>(source->materials.size()));
    for (const auto& mat : source->materials)
    {
        uint8_t* matBuf = nullptr;
        uint64_t matSize = saveMaterial(&mat, &matBuf);
        writer.u32(static_cast<uint32_t>(matSize));
        writer.bytes(matBuf, matSize);
        delete[] matBuf;
    }

    writer.u32(static_cast<uint32_t>(source->meshes.size()));
    for (const auto& mesh : source->meshes)
    {
        uint8_t* meshBuf = nullptr;
        uint64_t meshSize = saveMesh(&mesh, &meshBuf);
        writer.u32(static_cast<uint32_t>(meshSize));
        writer.bytes(meshBuf, meshSize);
        delete[] meshBuf;
    }

    *outBuffer = buffer;
    return size;
}

void ModelImporter::loadTyped(const uint8_t* buffer, ModelAsset* model)
{
    BinaryReader reader(buffer);

    model->m_uid = reader.u64();

    uint32_t materialCount = reader.u32();
    model->materials.resize(materialCount);

    for (uint32_t i = 0; i < materialCount; ++i)
    {
        uint32_t size = reader.u32();
        loadMaterial(reader.ptr(), &model->materials[i]);
        reader.skip(size);
    }

    uint32_t meshCount = reader.u32();
    model->meshes.resize(meshCount);

    for (uint32_t i = 0; i < meshCount; ++i)
    {
        uint32_t size = reader.u32();
        loadMesh(reader.ptr(), &model->meshes[i]);
        reader.skip(size);
    }
}

uint64_t ModelImporter::saveMesh(const MeshAsset* source, uint8_t** outBuffer)
{
    uint64_t size = 0;

    size += sizeof(uint64_t); // uid

    size += sizeof(uint32_t); // vertexCount
    size += source->vertices.size() * sizeof(Vertex);

    size += sizeof(uint32_t); // indexCount
    size += sizeof(uint32_t); // indexFormat
    size += source->indices.size() * sizeof(uint8_t);

    size += sizeof(uint32_t); // submeshCount
    size += source->submeshes.size() * sizeof(Submesh);

    size += sizeof(Vector3); // boundsCenter
    size += sizeof(Vector3); // boundsExtents

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter writer(buffer);

    writer.u64(source->m_uid);

    writer.u32(static_cast<uint32_t>(source->vertices.size()));
    writer.bytes(source->vertices.data(), source->vertices.size() * sizeof(Vertex));

    writer.u32(static_cast<uint32_t>(source->indices.size()));
    writer.u32(static_cast<uint32_t>(source->indexFormat));
    writer.bytes(source->indices.data(), source->indices.size());

    writer.u32(static_cast<uint32_t>(source->submeshes.size()));
    writer.bytes(source->submeshes.data(), source->submeshes.size() * sizeof(Submesh));

    writer.bytes(&source->boundsCenter, sizeof(Vector3));
    writer.bytes(&source->boundsExtents, sizeof(Vector3));

    *outBuffer = buffer;
    return size;
}

uint64_t ModelImporter::saveMaterial(const MaterialAsset* source, uint8_t** outBuffer)
{
    uint64_t size = 0;
    // Look a way to be able to get the size automatically
    size += sizeof(uint64_t); // uid
    size += sizeof(uint64_t); // baseMap
    size += sizeof(Color);    // baseColour
    size += sizeof(uint64_t); // metallicRoughnessMap
    size += sizeof(uint64_t); // metallicFactor
    size += sizeof(uint64_t); // normalMap
    size += sizeof(uint64_t); // occlusionMap
    size += sizeof(uint8_t);  // isEmissive
    size += sizeof(uint64_t); // emissiveMap

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter writer(buffer);

    writer.u64(source->m_uid);
    writer.u64(source->baseMap);
    writer.bytes(&source->baseColour, sizeof(Color));

    writer.u64(source->metallicRoughnessMap);
    writer.u32(source->metallicFactor);
    writer.u64(source->normalMap);
    writer.u64(source->occlusionMap);

    writer.u8(source->isEmissive ? 1 : 0);
    writer.u64(source->emissiveMap);

    *outBuffer = buffer;
    return size;
}

void ModelImporter::loadMesh(const uint8_t* buffer, MeshAsset* mesh)
{
    BinaryReader reader(buffer);

    mesh->m_uid = reader.u64();

    uint32_t vertexCount = reader.u32();
    mesh->vertices.resize(vertexCount);
    reader.bytes(mesh->vertices.data(), vertexCount * sizeof(Vertex));

    uint32_t indexCount = reader.u32();
    mesh->indexFormat = static_cast<DXGI_FORMAT>(reader.u32());
    mesh->indices.resize(indexCount);
    reader.bytes(mesh->indices.data(), indexCount);

    uint32_t submeshCount = reader.u32();
    mesh->submeshes.resize(submeshCount);
    reader.bytes(mesh->submeshes.data(), submeshCount * sizeof(Submesh));

    reader.bytes(&mesh->boundsCenter, sizeof(Vector3));
    reader.bytes(&mesh->boundsExtents, sizeof(Vector3));
}

void ModelImporter::loadMaterial(const uint8_t* buffer, MaterialAsset* material)
{
    BinaryReader reader(buffer);

    material->m_uid = reader.u64();
    material->baseMap = reader.u64();

    reader.bytes(&material->baseColour, sizeof(Color));

    material->metallicRoughnessMap = reader.u64();
    material->metallicFactor = reader.u32();
    material->normalMap = reader.u64();
    material->occlusionMap = reader.u64();

    material->isEmissive = reader.u8() != 0;
    material->emissiveMap = reader.u64();
}
