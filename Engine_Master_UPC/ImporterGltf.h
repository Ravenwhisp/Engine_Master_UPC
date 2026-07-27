#pragma once
#include "ImporterSource.h"
#include "UtilityGLFT.h"
#include "Metadata.h"
#include "AssetReference.h"

class AnimationAsset;
class SkinAsset;
class AnimationStateMachineAsset;
class Prefab;
class MaterialAsset;
class MeshAsset;
class GameObject;
class ImporterPrefab;

class ImporterGltf : public ImporterSource<tinygltf::Model, Prefab, AssetType::PREFAB>
{
public:

    ImporterGltf(Importer* importerMesh, Importer* importerMaterial, ImporterPrefab* importerPrefab,
        Importer* importerAnimation,
        Importer* importerSkin,
        Importer* importerAnimationStateMachine);

    bool   canImport(const std::filesystem::path& path) const override;
    Asset* createAssetInstance(AssetId& ref) const override;

    bool createStateMachine(const std::filesystem::path& gltfPath);

protected:
    bool     loadExternal(const std::filesystem::path& path, tinygltf::Model& out) override;
    void     importTyped(const tinygltf::Model& source, Prefab* model)        override;

private:
    void loadMaterial(const tinygltf::Model& model, const tinygltf::Material& material, MaterialAsset* materialAsset);
    void loadMesh(const tinygltf::Model& model, const tinygltf::Primitive& prim, MeshAsset* out, const AssetId& materialRef);
    void loadAnimation(const tinygltf::Model& model, const tinygltf::Animation& anim, AnimationAsset* outAnim);

    AssetId buildDefaultStateMachine(const tinygltf::Model& model, const std::vector<AssetId>& animationRefs, bool forceNew = false);

    void loadSkin(const tinygltf::Model& model,
        const tinygltf::Skin& skin,
        SkinAsset* outSkin);

    GameObject* makeNode(const std::string& name,
        std::vector<std::unique_ptr<GameObject>>& tempObjects) const;

    GameObject* buildNode(int nodeIdx,
        GameObject* parent,
        const tinygltf::Model& model,
        const std::vector<AssetId>& meshRefs,
        const std::vector<AssetId>& materialRefs,
        const std::vector<AssetId>& skinRefs,
        std::vector<std::unique_ptr<GameObject>>& tempObjects) const;


    AssetId resolveOrGenerateReference(AssetType type, const uint8_t* data, size_t size);

    AssetId resolveTexture(const tinygltf::Model& model, int texIndex) const;

    std::vector<DependencyRecord> m_existingDeps;
    std::vector<bool>             m_existingDepsUsed;

    const std::filesystem::path* m_currentFilePath = nullptr;

    Importer* m_importerMesh;
    Importer* m_importerMaterial;
    ImporterPrefab* m_importerPrefab;
    Importer* m_importerAnimation;
    Importer* m_importerSkin;
    Importer* m_importerAnimationStateMachine;
};
