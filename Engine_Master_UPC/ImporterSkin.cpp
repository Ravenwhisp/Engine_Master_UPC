#include "Globals.h"
#include "ImporterSkin.h"

#include "BinaryReader.h"
#include "BinaryWriter.h"

static uint64_t serializedStringSize(const std::string& s)
{
    return sizeof(uint32_t) + static_cast<uint64_t>(s.size());
}

bool ImporterSkin::importNative(const std::filesystem::path&, SkinAsset*)
{
    return false;
}

bool ImporterSkin::saveNative(const std::filesystem::path&, const SkinAsset*)
{
    return false;
}

uint64_t ImporterSkin::saveTyped(const SkinAsset* source, uint8_t** outBuffer)
{
    return CerealUtils::saveTo(*source, outBuffer);
}

void ImporterSkin::loadTyped(const uint8_t* buffer, SkinAsset* dst)
{
    CerealUtils::loadFrom(buffer, *dst);
}