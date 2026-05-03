#include "Globals.h"
#include "ImporterMaterial.h"

#include "MaterialAsset.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"

Asset* ImporterMaterial::createAssetInstance(AssetReference& uid) const

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

    writer.bytes(&source->baseMap, sizeof(AssetReference));
    writer.bytes(&source->baseColour, sizeof(Color));

    writer.bytes(&source->metallicRoughnessMap, sizeof(AssetReference));
    writer.u32(source->metallicFactor);
    writer.u32(source->roughnessFactor);
    writer.bytes(&source->normalMap, sizeof(AssetReference));
    writer.bytes(&source->occlusionMap, sizeof(AssetReference));

    writer.u8(source->isEmissive ? 1 : 0);
    writer.bytes(&source->emissiveMap, sizeof(AssetReference));

    *outBuffer = buffer;
    return size;
}

void ImporterMaterial::loadTyped(const uint8_t* buffer, MaterialAsset* material)
{
    BinaryReader reader(buffer);

    reader.bytes(&material->baseMap, sizeof(AssetReference));
    reader.bytes(&material->baseColour, sizeof(Color));

    reader.bytes(&material->metallicRoughnessMap, sizeof(AssetReference));
    material->metallicFactor = reader.u32();
    material->roughnessFactor = reader.u32();
    reader.bytes(&material->normalMap, sizeof(AssetReference));
    reader.bytes(&material->occlusionMap, sizeof(AssetReference));

    material->isEmissive = reader.u8() != 0;
    reader.bytes(&material->emissiveMap, sizeof(AssetReference));
}