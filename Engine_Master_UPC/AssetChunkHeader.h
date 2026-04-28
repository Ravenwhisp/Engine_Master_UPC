#pragma once
#include "UID.h"

struct AssetChunkHeader
{
    UID      localId;
    uint32_t type;
    uint64_t offset;
    uint64_t byteSize;
};