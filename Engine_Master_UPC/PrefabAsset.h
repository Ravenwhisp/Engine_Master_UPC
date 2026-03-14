#pragma once
#include "Asset.h"
#include <string>

// Instantiating the prefab into a scene is NOT the importer's responsibility.
// Use ModuleScene::instantiatePrefab(const PrefabAsset&) for that.
class PrefabAsset : public Asset
{
public:
    friend class ImporterPrefab;
    friend class ImporterGltf;

    PrefabAsset() {}
    PrefabAsset(MD5Hash id) : Asset(id, AssetType::PREFAB) {}

    // The serialized JSON body of the prefab, ready to parse at instantiation time.
    const std::string&      getJSON()  const { return m_json; }
    MD5Hash                 getRootUID() const { return m_rootUID; }

private:
    std::string     m_json;
    MD5Hash         m_rootUID = INVALID_ASSET_ID;
};