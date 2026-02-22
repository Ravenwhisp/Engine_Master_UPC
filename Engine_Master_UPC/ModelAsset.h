#pragma once
#include "Globals.h"
#include "Asset.h"
#include "Vertex.h"

class MaterialAsset : public Asset
{
public:
	friend class ModelImporter;
	MaterialAsset() {}
	MaterialAsset(int id) : Asset(id, AssetType::MATERIAL) {}

	uint32_t getBaseMap() const { return baseMap; }
	Color&	 getBaseColour() const { return baseColour; }
protected:

	uint32_t			baseMap = -1;
	mutable Color		baseColour = Color(255, 255, 255, 0);

	uint32_t			metallicRoughnessMap = -1;
	uint32_t			metallicFactor = 0;
	uint32_t			normalMap = -1;
	uint32_t			occlusionMap = -1;

	bool				isEmissive = false;
	uint32_t 			emissiveMap = -1;
};


struct Submesh
{
	uint32_t indexStart;
	uint32_t indexCount;
	uint32_t materialId;
};

class MeshAsset : public Asset
{
public:
	friend class ModelImporter;
	MeshAsset() {}
	MeshAsset(int id) : Asset(id, AssetType::MESH) {}

    const void* getVertexData() const { return vertices.data(); }
    uint32_t	getVertexCount() const {  return static_cast<uint32_t>(vertices.size()); }
    uint32_t	getVertexStride() const { return sizeof(Vertex); }

    const void* getIndexData() const { return indices.data(); }
    uint32_t	getIndexBufferSize() const { return static_cast<uint32_t>(indices.size()); }
	DXGI_FORMAT getIndexFormat() const { return indexFormat; }

    const std::vector<Submesh>& getSubmeshes() const { return submeshes; }
protected:
	std::vector<Vertex>		vertices;
	std::vector<uint8_t>	indices;
	DXGI_FORMAT				indexFormat = DXGI_FORMAT_R8_UINT;
	// The submeshes are tinygltf primitives, which are the smallest renderable units.
	std::vector<Submesh>	submeshes;

	Vector3 boundsCenter;
	Vector3 boundsExtents;
};

class ModelAsset : public Asset
{
public:
	friend class ModelImporter;
	ModelAsset() {}
	ModelAsset(int id) : Asset(id, AssetType::MODEL) {}

	std::vector<MeshAsset>& getMeshes() const { return meshes; }
	std::vector<MaterialAsset>& getMaterials() const { return materials; }
protected:
	mutable std::vector<MeshAsset> meshes;
	mutable std::vector<MaterialAsset> materials;
};