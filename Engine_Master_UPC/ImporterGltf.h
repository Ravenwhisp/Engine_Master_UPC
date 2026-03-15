#pragma once
#include "ImporterSource.h"
#include "UtilityGLFT.h"
#include <ImporterMesh.h>
#include <ImporterMaterial.h>

class PrefabAsset;
class MaterialAsset;
class MeshAsset;
class GameObject;



// Handles the full import pipeline for .gltf source files.
// Mesh and material details are delegated to MeshImporter and MaterialImporter.
class ImporterGltf : public ImporterSource<tinygltf::Model, PrefabAsset, AssetType::PREFAB>
{
public:

    ImporterGltf(ImporterMesh& ImporterMesh, ImporterMaterial& importerMaterial);

    bool   canImport(const std::filesystem::path& path) const override;
    Asset* createAssetInstance(const MD5Hash& uid) const override;

protected:
    bool     loadExternal(const std::filesystem::path& path, tinygltf::Model& out) override;
    void     importTyped(const tinygltf::Model& source, PrefabAsset* model)        override;
    uint64_t saveTyped(const PrefabAsset* model, uint8_t** outBuffer)            override;
    void     loadTyped(const uint8_t* buffer, PrefabAsset* model)              override;

private:
    // Kept for the duration of a single import call; reset to nullptr afterwards.
    void loadMaterial(const tinygltf::Model& model, const tinygltf::Material& material, MaterialAsset* materialAsset);
    void loadMesh(const tinygltf::Model& model, const tinygltf::Primitive& prim, MeshAsset* out, const MD5Hash& materialUID);

    const std::filesystem::path*    m_currentFilePath = nullptr;

    ImporterMesh&                   m_importerMesh;
    ImporterMaterial&               m_importerMaterial;
};