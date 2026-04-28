#include "Globals.h"
#include "ImporterMaterial.h"

#include "MaterialAsset.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"

Asset* ImporterMaterial::createAssetInstance(const UID& uid) const

{
    return new MaterialAsset(uid);
}


bool ImporterMaterial::importNative(const std::filesystem::path& path, MaterialAsset* dst)
{
	return false;
}

bool ImporterMaterial::saveNative(const std::filesystem::path& path, const MaterialAsset* src)
{
    return false;
}

static uint64_t stringSerialSize(const std::string& s)
{
    return sizeof(uint32_t) + s.size();
}

uint64_t ImporterMaterial::saveTyped(const MaterialAsset* source, uint8_t** outBuffer)
{
    return CerealUtils::saveTo(*source, outBuffer);
}

void ImporterMaterial::loadTyped(const uint8_t* buffer, MaterialAsset* material)
{
    CerealUtils::loadFrom(buffer, *material);
}