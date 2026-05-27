#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

enum class ArchiveMode { Input, Output };

class IArchive
{
public:
    virtual ~IArchive() = default;
    virtual ArchiveMode mode() const = 0;

    virtual void serialize(uint8_t& val, const char* name = "") = 0;
    virtual void serialize(uint32_t& val, const char* name = "") = 0;
    virtual void serialize(uint64_t& val, const char* name = "") = 0;
    virtual void serialize(float& val, const char* name = "") = 0;
    virtual void serialize(bool& val, const char* name = "") = 0;
    virtual void serialize(std::string& val, const char* name = "") = 0;
    virtual void serializeRaw(void* data, size_t size, const char* name = "") = 0;
};
