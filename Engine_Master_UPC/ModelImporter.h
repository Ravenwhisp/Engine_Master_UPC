#pragma once

#include "TypedImporter.h"
#include <ResourcesModule.h>
#include "UtilityGLFT.h"
#include <ModelAsset.h>


// We can also treat ModelImporter as a GLTF file loader, which loads the entire file into a Model structure. 
class ModelImporter : public TypedImporter<tinygltf::Model, ModelAsset>
{
public:
	bool canImport(const std::filesystem::path& path) const override
	{
		auto ext = path.extension().string();
		return ext == ".gltf";
	}

	Asset* createAssetInstance(int uid) const override
	{
		return new ModelAsset(uid);
	}

protected:
	bool loadExternal(const std::filesystem::path& path, tinygltf::Model& out) override;
	void importTyped(const tinygltf::Model& source, ModelAsset* model) override;
	uint64_t saveTyped(const ModelAsset* model, uint8_t** buffer) override;
	void loadTyped(const uint8_t* buffer, ModelAsset* model) override;
private:
	void loadMaterial(const tinygltf::Model& model, const tinygltf::Material& material, MaterialAsset* materialAsset);
	void loadMesh(const tinygltf::Model& model, const tinygltf::Primitive& primitive, MeshAsset* mesh, const std::vector<uint32_t>& materialRemap);
};