#include "Globals.h"
#include "AssetReference.h"

void AssetReference::serialize(IArchive& archive)
{
    archive.serialize(m_uid, "uid");
    archive.serialize(m_libId, "libId");
    archive.serializeStringEnum(m_type, "type", AssetTypeToString, StringToAssetType);
}


