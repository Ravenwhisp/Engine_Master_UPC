#include "Globals.h"
#include "ImporterMaterial.h"

#include "MaterialAsset.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"

Asset* ImporterMaterial::createAssetInstance(const MD5Hash& uid) const

{
    return new MaterialAsset(uid);
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

    size += stringSerialSize(source->m_uid);
    size += stringSerialSize(source->baseMap);
    size += sizeof(Color);
    size += stringSerialSize(source->metallicRoughnessMap);
    size += sizeof(uint32_t);                               // metallicFactor
    size += stringSerialSize(source->normalMap);
    size += stringSerialSize(source->occlusionMap);
    size += sizeof(uint8_t);                                // isEmissive
    size += stringSerialSize(source->emissiveMap);

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter writer(buffer);

    writer.string(source->m_uid);
    writer.string(source->baseMap);
    writer.bytes(&source->baseColour, sizeof(Color));

    writer.string(source->metallicRoughnessMap);
    writer.u32(source->metallicFactor);
    writer.string(source->normalMap);
    writer.string(source->occlusionMap);

    writer.u8(source->isEmissive ? 1 : 0);
    writer.string(source->emissiveMap);

    *outBuffer = buffer;
    return size;
}

void ImporterMaterial::loadTyped(const uint8_t* buffer, MaterialAsset* material)
{
    BinaryReader reader(buffer);

    material->m_uid = reader.string();
    material->baseMap = reader.string();
    reader.bytes(&material->baseColour, sizeof(Color));

    material->metallicRoughnessMap = reader.string();
    material->metallicFactor = reader.u32();
    material->normalMap = reader.string();
    material->occlusionMap = reader.string();

    material->isEmissive = reader.u8() != 0;
    material->emissiveMap = reader.string();
}