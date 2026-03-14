#pragma once
#include "Globals.h"
#include "Asset.h"
#include "IndexBuffer.h"



class ModelAsset : public Asset
{
public:
	friend class ModelImporter;
	ModelAsset() {}
	ModelAsset(MD5Hash id) : Asset(id, AssetType::MODEL) {}

	std::vector<MeshAsset>& getMeshes() const { return meshes; }
	std::vector<MaterialAsset>& getMaterials() const { return materials; }
protected:
	mutable std::vector<MeshAsset> meshes;
	mutable std::vector<MaterialAsset> materials;
};