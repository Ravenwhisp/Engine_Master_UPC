#include "Globals.h"
#include "ImporterAnimation.h"

#include "BinaryReader.h"
#include "BinaryWriter.h"


static uint64_t strSerialSize(const std::string& s)
{
    return sizeof(uint32_t) + s.size();
}

bool ImporterAnimation::importNative(const std::filesystem::path&, AnimationAsset*)
{
    return false;
}

bool ImporterAnimation::saveNative(const std::filesystem::path&, const AnimationAsset*)
{
    return false;
}

uint64_t ImporterAnimation::saveTyped(const AnimationAsset* a, uint8_t** outBuffer)
{
    return CerealUtils::saveTo(*a, outBuffer);
}

void ImporterAnimation::loadTyped(const uint8_t* buffer, AnimationAsset* a)
{
    return CerealUtils::loadFrom(buffer, *a);
}