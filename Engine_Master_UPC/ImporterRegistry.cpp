#include "Globals.h"
#include "ImporterRegistry.h"

#include "ImporterTexture.h"
#include "ImporterMesh.h"
#include "ImporterMaterial.h"
#include "ImporterPrefab.h"
#include "ImporterAnimation.h"
#include "ImporterSkin.h"
#include "ImporterAnimationStateMachine.h"
#include "ImporterGltf.h"
#include "ImporterFont.h"
#include "ImporterScene.h"

ImporterRegistry::ImporterRegistry()
{
    auto importerMesh = std::make_unique<ImporterMesh>();
    auto importerMaterial = std::make_unique<ImporterMaterial>();
    auto importerPrefab = std::make_unique<ImporterPrefab>();
    auto importerAnimation = std::make_unique<ImporterAnimation>();
    auto importerSkin = std::make_unique<ImporterSkin>();
    auto importerAnimStateMachine = std::make_unique<ImporterAnimationStateMachine>();

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
    m_importers.push_back(std::make_unique<ImporterScene>());
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
