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

	std::shared_ptr<Asset> createAssetInstance(UID uid) const override
	{
		return std::make_shared<ModelAsset>(uid);
	}

protected:
	bool loadExternal(const std::filesystem::path& path, tinygltf::Model& out) override;
	void importTyped(const tinygltf::Model& source, std::shared_ptr<ModelAsset> model) override;
	uint64_t saveTyped(const std::shared_ptr<ModelAsset> model, uint8_t** buffer) override;
	void loadTyped(const uint8_t* buffer, std::shared_ptr<ModelAsset> model) override;
private:
	uint64_t saveMesh(const std::shared_ptr<MeshAsset> mesh, uint8_t** buffer);
	uint64_t saveMaterial(const std::shared_ptr<MaterialAsset> material, uint8_t** buffer);

	void loadMesh(const uint8_t* buffer, std::shared_ptr<MeshAsset> mesh);
	void loadMaterial(const uint8_t* buffer, std::shared_ptr<MaterialAsset> material);

	void loadMaterial(const tinygltf::Model& model, const tinygltf::Material& material, std::shared_ptr<MaterialAsset> materialAsset);
	void loadMesh(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::shared_ptr<MeshAsset> mesh, const std::vector<uint32_t>& materialRemap);

	const std::filesystem::path* m_currentFilePath;
};