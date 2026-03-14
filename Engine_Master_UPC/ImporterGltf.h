#pragma once
#include "ImporterSource.h"
#include "UtilityGLFT.h"

class ModelAsset;
class MaterialAsset;
class MeshAsset;
class GameObject;

constexpr const char* GLTF_EXTENSION = ".gltf";

// Handles the full import pipeline for .gltf source files.
// Mesh and material details are delegated to MeshImporter and MaterialImporter.
class ImporterGltf : public ImporterSource<tinygltf::Model, ModelAsset, AssetType::MODEL>
{
public:
    bool   canImport(const std::filesystem::path& path) const override;
    Asset* createAssetInstance(const MD5Hash& uid) const override;

protected:
    bool     loadExternal(const std::filesystem::path& path, tinygltf::Model& out) override;
    void     importTyped(const tinygltf::Model& source, ModelAsset* model)        override;
    uint64_t saveTyped(const ModelAsset* model, uint8_t** outBuffer)            override;
    void     loadTyped(const uint8_t* buffer, ModelAsset* model)              override;

private:
    // Kept for the duration of a single import call; reset to nullptr afterwards.
    void loadMaterial(const tinygltf::Model& model, const tinygltf::Material& material, MaterialAsset* materialAsset);
    void loadMesh(const tinygltf::Model& model, const tinygltf::Primitive& primitive, MeshAsset* mesh, const std::vector<MD5Hash>& materialRemap);

    const std::filesystem::path* m_currentFilePath = nullptr;
};