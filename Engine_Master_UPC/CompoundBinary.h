#pragma once
#include "UID.h"
#include <vector>
#include "AssetType.h"
#include "AssetChunkHeader.h"

struct CompoundBinary
{
    void addChunk(const UID& localId, AssetType type, const uint8_t* data, uint64_t size);
    std::vector<uint8_t> finalise() const;

    bool parse(const uint8_t* buffer, uint64_t size);
    const uint8_t* findChunk(const UID& localId, uint64_t& outSize) const;

    AssetType getChunkType(const UID& localId);
private:
    struct Entry 
    { 
        AssetChunkHeader header; 
        std::vector<uint8_t> blob; 
    };
    std::vector<Entry> m_entries;

    const uint8_t* m_base = nullptr;
    std::vector<AssetChunkHeader> m_parsedHeaders;
};