#pragma once

#include "TypedImporter.h"
#include <ResourcesModule.h>
#include "UtilityGLFT.h"
#include <ModelAsset.h>

constexpr const char* GLTF_EXTENSION = ".gltf";

// We can also treat ModelImporter as a GLTF file loader, which loads the entire file into a Model structure. 
class ModelImporter : public TypedImporter<tinygltf::Model, ModelAsset>
{
public:
	bool canImport(const std::filesystem::path& path) const override
	{
		auto ext = path.extension().string();
		return ext == GLTF_EXTENSION;
	}

	Asset* createAssetInstance(UID uid) const override
	{
		return new ModelAsset(uid);
	}

protected:
	bool loadExternal(const std::filesystem::path& path, tinygltf::Model& out) override;
	void importTyped(const tinygltf::Model& source, ModelAsset* model) override;
	uint64_t saveTyped(const ModelAsset* model, uint8_t** buffer) override;
	void loadTyped(const uint8_t* buffer, ModelAsset* model) override;
private:
	uint64_t saveMesh(const MeshAsset* mesh, uint8_t** buffer);
	uint64_t saveMaterial(const MaterialAsset* material, uint8_t** buffer);

	void loadMesh(const uint8_t* buffer, MeshAsset* mesh);
	void loadMaterial(const uint8_t* buffer, MaterialAsset* material);

	void loadMaterial(const tinygltf::Model& model, const tinygltf::Material& material, MaterialAsset* materialAsset);
	void loadMesh(const tinygltf::Model& model, const tinygltf::Primitive& primitive, MeshAsset* mesh, const std::vector<uint32_t>& materialRemap);

	const std::filesystem::path* m_currentFilePath;
};