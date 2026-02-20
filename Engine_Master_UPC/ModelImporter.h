#pragma once

#include "TypedImporter.h"
#include <ResourcesModule.h>

#include "UtilityGLFT.h"

// We should store the id of the texture in the material, so that we can load the texture when we load the material.
class MaterialAsset: public Asset
{
public:
	friend class ModelImporter;

	MaterialAsset(int id) : Asset(id) {}
protected:

	uint32_t	baseMap;
	Color		baseColour = Color(255,255,255,0);

	uint32_t	metallicRoughnessMap;
	uint32_t	metallicFactor = 0;
	uint32_t	normalMap;
	uint32_t	occlusionMap;

	bool		isEmissive = false;
	uint32_t 	emissiveMap;
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

	MeshAsset(int id) : Asset(id) {}
protected:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	// The submeshes are tinygltf primitives, which are the smallest renderable units.
	std::vector<Submesh> submeshes;

	Vector3 boundsCenter;
	Vector3 boundsExtents;
};

class ModelAsset : public Asset
{
public:
	friend class ModelImporter;
	ModelAsset(int id) : Asset(id) {}

protected:

	std::vector<MeshAsset> meshes;
	std::vector<MaterialAsset> materials;
};


// We can also treat ModelImporter as a GLTF file loader, which loads the entire file into a Model structure. 
class ModelImporter : public TypedImporter<tinygltf::Model, ModelAsset>
{
public:
	bool canImport(const std::filesystem::path& path) const override
	{
		auto ext = path.extension().string();
		return ext == ".gltf";
	}

	Asset* createAssetInstance() const override
	{
		return new ModelAsset(rand());
	}

protected:
	bool loadExternal(const std::filesystem::path& path, tinygltf::Model& out) override;
	void importTyped(const tinygltf::Model& source, ModelAsset* model) override;
	uint64_t saveTyped(const ModelAsset* model, uint8_t** buffer) override;
	void loadTyped(const uint8_t* buffer, ModelAsset* model) override;
private:
	void loadMaterial(const tinygltf::Model& model, const tinygltf::Material& material, MaterialAsset* materialAsset);
	void loadMesh(const tinygltf::Model& model, const tinygltf::Primitive& primitive, MeshAsset* mesh);
};