#include "Globals.h"
#include "AssetId.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "AssetIndex.h"

#include <unordered_set>

namespace
{
    // Asset library files are named by content hash. Serialized references
    // store the libId they had when they were saved, so a reimport of the
    // referenced asset makes every persisted copy stale. The uid is the real
    // identity of an asset, so before an id is written to (or read from) any
    // archive, sync its libId against the asset index.
    void healAssetId(AssetId& id)
    {
        if (!app || !app->getModuleAssets())
            return;

        if (!id.hasUID())
            return;

        const AssetIndex& index = app->getModuleAssets()->getIndex();
        if (index.size() == 0)
            return;

        const AssetIndexEntry* entry = index.findEntry(id.m_uid);
        if (!entry)
        {
            // Dangling reference (asset deleted / never imported). Do not
            // destroy the stored data, but surface it. Skipped when the index
            // is empty (e.g. GAME_RELEASE runs without an index).
            static std::unordered_set<UID> s_missingWarned;
            if (s_missingWarned.insert(id.m_uid).second)
            {
                DEBUG_WARN("[AssetId] uid %llu not found in the asset index; reference left untouched.",
                           (unsigned long long)id.m_uid);
            }
            return;
        }

        if (!isValidAsset(entry->contentHash) || entry->contentHash == id.m_libId)
            return;

        static std::unordered_set<UID> s_healedWarned;
        if (s_healedWarned.insert(id.m_uid).second)
        {
            DEBUG_WARN("[AssetId] healed reference uid %llu: stale libId '%s' -> '%s'.",
                       (unsigned long long)id.m_uid, id.m_libId.c_str(), entry->contentHash.c_str());
        }

        id.m_libId = entry->contentHash;
    }
}

void AssetId::serialize(IArchive& archive)
{
    if (archive.mode() == ArchiveMode::Input)
    {
        archive.serialize(m_uid, "uid");
        archive.serialize(m_libId, "libId");
        archive.serializeStringEnum(m_type, "type", AssetTypeToString, StringToAssetType);

        // Fix references as they come in, so in-memory data is correct right
        // after a load without any extra pass.
        healAssetId(*this);
    }
    else
    {
        // Fix references before they are persisted, so anything we save
        // (prefab re-bakes, scenes, component data) carries the current
        // library hash of the referenced asset.
        healAssetId(*this);

        archive.serialize(m_uid, "uid");
        archive.serialize(m_libId, "libId");
        archive.serializeStringEnum(m_type, "type", AssetTypeToString, StringToAssetType);
    }
}
