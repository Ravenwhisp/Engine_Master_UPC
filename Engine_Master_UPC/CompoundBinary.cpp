#include "Globals.h"
#include "CompoundBinary.h"

void CompoundBinary::addChunk(const UID& localId, AssetType type, const uint8_t* data, uint64_t size)
{
}

std::vector<uint8_t> CompoundBinary::finalise() const
{
    return std::vector<uint8_t>();
}

bool CompoundBinary::parse(const uint8_t* buffer, uint64_t size)
{
    return false;
}

const uint8_t* CompoundBinary::findChunk(const UID& localId, uint64_t& outSize) const
{
    return nullptr;
}

AssetType CompoundBinary::getChunkType(const UID& localId)
{
    return AssetType();
}
