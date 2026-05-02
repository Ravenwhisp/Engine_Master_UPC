#include "Globals.h"
#include "ImporterMaterial.h"

#include "MaterialAsset.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"

Asset* ImporterMaterial::createAssetInstance(const UID& uid) const

{
    return new MaterialAsset(uid);
}

bool ImporterMaterial::saveNative(const MaterialAsset* asset, const std::filesystem::path& path)
{
    return false;
}


bool ImporterMaterial::importNative(const std::filesystem::path& path, MaterialAsset* dst)
{
	return false;
}

static uint64_t stringSerialSize(const std::string& s)
{
    return sizeof(uint32_t) + s.size();
}

uint64_t ImporterMaterial::saveTyped(const MaterialAsset* source, uint8_t** outBuffer)
{
    uint64_t size = 0;

    size += sizeof(source->baseMap);
    size += sizeof(Color);
    size += sizeof(source->metallicRoughnessMap);
    size += sizeof(uint32_t);                               // metallicFactor
    size += sizeof(uint32_t);                               // roughnessFactor
    size += sizeof(source->normalMap);
    size += sizeof(source->occlusionMap);
    size += sizeof(uint8_t);                                // isEmissive
    size += sizeof(source->emissiveMap);

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter writer(buffer);

    writer.u64(source->baseMap);
    writer.bytes(&source->baseColour, sizeof(Color));

    writer.u64(source->metallicRoughnessMap);
    writer.u32(source->metallicFactor);
    writer.u32(source->roughnessFactor);
    writer.u64(source->normalMap);
    writer.u64(source->occlusionMap);

    writer.u8(source->isEmissive ? 1 : 0);
    writer.u64(source->emissiveMap);

    *outBuffer = buffer;
    return size;
}

void ImporterMaterial::loadTyped(const uint8_t* buffer, MaterialAsset* material)
{
    BinaryReader reader(buffer);

    material->baseMap = reader.u64();
    reader.bytes(&material->baseColour, sizeof(Color));

    material->metallicRoughnessMap = reader.u64();
    material->metallicFactor = reader.u32();
    material->roughnessFactor = reader.u32();
    material->normalMap = reader.u64();
    material->occlusionMap = reader.u64();

    material->isEmissive = reader.u8() != 0;
    material->emissiveMap = reader.u64();
}