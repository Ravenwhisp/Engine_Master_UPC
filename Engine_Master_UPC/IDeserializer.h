#pragma once
#include <string>
#include <cstdint>
#include <functional>

class IDeserializer
{
public:
    virtual ~IDeserializer() = default;

    virtual void read(const char* key, bool& value) = 0;
    virtual void read(const char* key, int32_t& value) = 0;
    virtual void read(const char* key, uint32_t& value) = 0;
    virtual void read(const char* key, uint64_t& value) = 0;
    virtual void read(const char* key, float& value) = 0;
    virtual void read(const char* key, std::string& value) = 0;

    virtual void readBytes(const char* key, void* dst, size_t byteCount) = 0;

    virtual void readObject(const char* key, const std::function<void(IDeserializer&)>& fn) = 0;

    virtual size_t readArray(const char* key, const std::function<void(IDeserializer&, size_t index)>& fn) = 0;
};