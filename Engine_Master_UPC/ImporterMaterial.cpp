#include "Globals.h"
#include "ImporterMaterial.h"

#include "MaterialAsset.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"

static void writeAssetReference(BinaryWriter& writer, const AssetReference& ref)
{
    writer.u64(ref.m_uid);
    writer.string(ref.m_libId);
    writer.u32(static_cast<uint32_t>(ref.m_type));
}

static void readAssetReference(BinaryReader& reader, AssetReference& ref)
{
    ref.m_uid = reader.u64();
    ref.m_libId = reader.string();
    ref.m_type = static_cast<AssetType>(reader.u32());
}

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
    // Helper to compute the serialized size of one AssetReference
    auto refSize = [](const AssetReference& ref) -> uint64_t {
        return sizeof(uint64_t)              // m_uid
            + sizeof(uint32_t)              // m_libId length prefix
            + ref.m_libId.size()            // m_libId content
            + sizeof(uint32_t);             // m_type
        };

    uint64_t size = 0;
    size += refSize(source->baseMap);
    size += sizeof(Color);
    size += refSize(source->metallicRoughnessMap);
    size += sizeof(uint32_t);                // metallicFactor
    size += sizeof(uint32_t);                // roughnessFactor
    size += refSize(source->normalMap);
    size += refSize(source->occlusionMap);
    size += sizeof(uint8_t);                 // isEmissive
    size += refSize(source->emissiveMap);

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter writer(buffer);

    writeAssetReference(writer, source->baseMap);
    writer.bytes(&source->baseColour, sizeof(Color));
    writeAssetReference(writer, source->metallicRoughnessMap);
    writer.u32(source->metallicFactor);
    writer.u32(source->roughnessFactor);
    writeAssetReference(writer, source->normalMap);
    writeAssetReference(writer, source->occlusionMap);
    writer.u8(source->isEmissive ? 1 : 0);
    writeAssetReference(writer, source->emissiveMap);

    *outBuffer = buffer;
    return size;
}

void ImporterMaterial::loadTyped(const uint8_t* buffer, MaterialAsset* material)
{
    BinaryReader reader(buffer);

    readAssetReference(reader, material->baseMap);
    reader.bytes(&material->baseColour, sizeof(Color));
    readAssetReference(reader, material->metallicRoughnessMap);
    material->metallicFactor = reader.u32();
    material->roughnessFactor = reader.u32();
    readAssetReference(reader, material->normalMap);
    readAssetReference(reader, material->occlusionMap);
    material->isEmissive = reader.u8() != 0;
    readAssetReference(reader, material->emissiveMap);
}