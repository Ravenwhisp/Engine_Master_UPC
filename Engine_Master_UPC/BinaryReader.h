#pragma once
#include <cstdint>
#include <cstring>
#include <string>

class BinaryReader {
public:
    BinaryReader(const uint8_t* buffer) : cursor(buffer) {}

    uint32_t u32() 
    {
        uint32_t v;
        std::memcpy(&v, cursor, sizeof(v));
        cursor += sizeof(v);
        return v;
    }

    uint64_t u64() 
    {
        uint64_t v;
        std::memcpy(&v, cursor, sizeof(v));
        cursor += sizeof(v);
        return v;
    }

    void bytes(void* dst, size_t size) 
    {
        std::memcpy(dst, cursor, size);
        cursor += size;
    }

    std::string string() {
        uint32_t len = u32();
        std::string s(len, '\0');
        bytes(s.data(), len);
        return s;
    }

private:
    const uint8_t* cursor;
};