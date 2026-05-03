#pragma once
#include "Asset.h"

#include <filesystem>
#include <string>

// ============================================================================
// PrefabData
// Single source of truth for all prefab data — asset identity, persistence
// payload, and per-instance runtime overrides.
//
//   m_sourcePath — full path to the source file (.prefab or .gltf).
//                  This is the ONLY field used for file-system operations.
//                  PrefabManager never reconstructs the path from a name.
//
//   m_name       — display name (path stem); used in the editor UI and
//                  embedded in m_json for human readability.
//                  NOT used for any file-system operation.
//
//   m_assetUID   — MD5Hash key used by ModuleAssets for cache lookup.
//
//   m_prefabUID  — UID of the root GameObject that was passed to createPrefab.
//                  This is a uint64_t engine UID, NOT a name-derived hash.
//                  Stored in scene JSON "PrefabLink.PrefabUID" so that live
//                  instances can be associated back to their source prefab even
//                  after the file is moved (as long as the UID is known).
//
//   m_json       — Complete prefab document:
//                    { "SourcePath", "Name", "Version",
//                      "PrefabUID", "GameObject": {...} }
//                  This is the persistence payload.  Read from disk verbatim by
//                  ImporterPrefab; produced by ImporterGltf via
//                  PrefabManager::buildPrefabJSON.
//
//   m_overrides  — Runtime-only; always empty on the asset itself.
//                  NOT written to the binary cache.
// ============================================================================
struct PrefabData
{
    // ── Identity ──────────────────────────────────────────────────────────────
    std::filesystem::path m_sourcePath;      // full canonical path; used for all I/O
    std::string           m_name;            // display name = stem of m_sourcePath
    MD5Hash               m_assetUID;        // asset system key

    // ── Persistence payload ───────────────────────────────────────────────────
    std::string m_json;

};

// ============================================================================
// PrefabAsset
// Thin Asset wrapper around PrefabData.
// Use PrefabManager::instantiatePrefab(asset, scene) to spawn into a scene.
// ============================================================================
class PrefabAsset : public Asset
{
public:
    friend class ImporterPrefab;
    friend class ImporterGltf;

    PrefabAsset() = default;
    explicit PrefabAsset(MD5Hash id) : Asset(id, AssetType::PREFAB)
    {
        m_data.m_assetUID = id;
    }

    PrefabData& getData() { return m_data; }
    const PrefabData& getData() const { return m_data; }

    // Convenience accessors.
    const std::string& getJSON()       const { return m_data.m_json; }
    const std::string& getName()       const { return m_data.m_name; }
    const std::filesystem::path& getSourcePath() const { return m_data.m_sourcePath; }
    MD5Hash                      getAssetUID()   const { return m_data.m_assetUID; }
private:
    PrefabData m_data;
};