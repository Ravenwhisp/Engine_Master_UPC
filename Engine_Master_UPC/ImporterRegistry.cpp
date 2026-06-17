#include "Globals.h"
#include "ImporterRegistry.h"

#include "ImporterNative.h"
#include "ImporterPrefab.h"
#include "ImporterTexture.h"
#include "ImporterGltf.h"
#include "ImporterFont.h"

#include "Scene.h"
#include "Prefab.h"
#include "MeshAsset.h"
#include "MaterialAsset.h"
#include "AnimationAsset.h"
#include "SkinAsset.h"
#include "AnimationStateMachineAsset.h"

ImporterRegistry::ImporterRegistry()
{
    auto importerMesh = std::make_unique<ImporterNative<MeshAsset, AssetType::MESH>>(std::initializer_list<const char*>{MESH_EXTENSION});
    auto importerMaterial = std::make_unique<ImporterNative<MaterialAsset, AssetType::MATERIAL>>(std::initializer_list<const char*>{MATERIAL_EXTENSION});
    auto importerPrefab = std::make_unique<ImporterPrefab>();
    auto importerAnimation = std::make_unique<ImporterNative<AnimationAsset, AssetType::ANIMATION>>(std::initializer_list<const char*>{});
    auto importerSkin = std::make_unique<ImporterNative<SkinAsset, AssetType::SKIN>>(std::initializer_list<const char*>{});
    auto importerAnimStateMachine = std::make_unique<ImporterNative<AnimationStateMachineAsset, AssetType::ANIMATION_STATE_MACHINE>>(std::initializer_list<const char*>{ANIMATION_STATE_MACHINE_EXTENSION});

    auto importerGltf = std::make_unique<ImporterGltf>(importerMesh.get(), importerMaterial.get(), importerPrefab.get(), importerAnimation.get(), importerSkin.get(), importerAnimStateMachine.get());
    m_importerGltfPtr = importerGltf.get();

    m_importers.push_back(std::make_unique<ImporterTexture>());
    m_importers.push_back(std::move(importerMesh));
    m_importers.push_back(std::move(importerMaterial));
    m_importers.push_back(std::move(importerPrefab));
    m_importers.push_back(std::move(importerAnimation));
    m_importers.push_back(std::move(importerSkin));
    m_importers.push_back(std::move(importerAnimStateMachine));
    m_importers.push_back(std::move(importerGltf));

    m_importers.push_back(std::make_unique<ImporterFont>());
    m_importers.push_back(std::make_unique<ImporterNative<Scene, AssetType::SCENE>>(std::initializer_list<const char*>{SCENE_EXTENSION}));
}

ImporterRegistry::~ImporterRegistry() = default;

Importer* ImporterRegistry::findByPath(const std::filesystem::path& filePath) const
{
    for (auto& importer : m_importers)
    {
        if (importer->canImport(filePath)) return importer.get();
    }
    return nullptr;
}

Importer* ImporterRegistry::findByType(AssetType type) const
{
    for (auto& importer : m_importers)
    {
        if (importer->getAssetType() == type) return importer.get();
    }
    return nullptr;
}

bool ImporterRegistry::canImport(const std::filesystem::path& sourcePath) const
{
    return findByPath(sourcePath) != nullptr;
}
