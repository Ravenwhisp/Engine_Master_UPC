#pragma once
#include <string>
#include <cstdint>
#include <functional>

class ISerializer
{
public:
    virtual ~ISerializer() = default;

    virtual void write(const char* key, bool value) = 0;
    virtual void write(const char* key, int32_t value) = 0;
    virtual void write(const char* key, uint32_t value) = 0;
    virtual void write(const char* key, uint64_t value) = 0;
    virtual void write(const char* key, float value) = 0;
    virtual void write(const char* key, const std::string& value) = 0;

    virtual void writeBytes(const char* key, const void* data, size_t byteCount) = 0;

    virtual void writeObject(const char* key, const std::function<void(ISerializer&)>& fn) = 0;

    virtual void writeArray(const char* key, size_t count, const std::function<void(ISerializer&, size_t index)>& fn) = 0;
};
