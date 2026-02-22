#pragma once
#include <cstdint>
#include <cstring>
#include <string>

class BinaryWriter {
public:
    BinaryWriter(uint8_t* buffer) : cursor(buffer) {}

    void u8(uint8_t v) {
        std::memcpy(cursor, &v, sizeof(v));
        cursor += sizeof(v);
    }

    void u32(uint32_t v) 
    {
        std::memcpy(cursor, &v, sizeof(v));
        cursor += sizeof(v);
    }

    void u64(uint64_t v) 
    {
        std::memcpy(cursor, &v, sizeof(v));
        cursor += sizeof(v);
    }

    void bytes(const void* data, size_t size) 
    {
        std::memcpy(cursor, data, size);
        cursor += size;
    }

    void string(const std::string& s) 
    {
        u32(static_cast<uint32_t>(s.size()));
        bytes(s.data(), s.size());
    }

private:
    uint8_t* cursor;
};