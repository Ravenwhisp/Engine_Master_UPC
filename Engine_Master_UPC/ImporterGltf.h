#pragma once
#include "ImporterSource.h"
#include "UtilityGLFT.h"

class AnimationAsset;
class SkinAsset;
class PrefabAsset;
class MaterialAsset;
class MeshAsset;
class GameObject;
class ImporterMesh;
class ImporterMaterial;
class ImporterPrefab;
class ImporterAnimation;
class ImporterSkin;

// Handles the full import pipeline for .gltf source files.
// Mesh and material details are delegated to MeshImporter and MaterialImporter.
class ImporterGltf : public ImporterSource<tinygltf::Model, PrefabAsset, AssetType::PREFAB>
{
public:

    ImporterGltf(ImporterMesh& importerMesh,
        ImporterMaterial& importerMaterial,
        ImporterPrefab& importerPrefab,
        ImporterAnimation& importerAnimation,
        ImporterSkin& importerSkin);

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
    void loadAnimation(const tinygltf::Model& model,
        const tinygltf::Animation& anim,
        AnimationAsset* outAnim);

    void loadSkin(const tinygltf::Model& model,
        const tinygltf::Skin& skin,
        SkinAsset* outSkin);

    GameObject* makeNode(const std::string& name,
        std::vector<std::unique_ptr<GameObject>>& tempObjects) const;

    GameObject* buildNode(int nodeIdx,
        GameObject* parent,
        const tinygltf::Model& model,
        const std::vector<MD5Hash>& meshUIDs,
        const std::vector<MD5Hash>& materialUIDs,
        const std::vector<MD5Hash>& skinUIDs,
        std::vector<std::unique_ptr<GameObject>>& tempObjects) const;

    const std::filesystem::path*    m_currentFilePath = nullptr;

    ImporterMesh&                   m_importerMesh;
    ImporterMaterial&               m_importerMaterial;
    ImporterPrefab&                 m_importerPrefab;
    ImporterAnimation&              m_importerAnimation;
    ImporterSkin&                   m_importerSkin;
};