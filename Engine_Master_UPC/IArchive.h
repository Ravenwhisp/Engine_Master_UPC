#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include "SimpleMath.h"

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

    virtual void serialize(DirectX::SimpleMath::Vector3& val, const char* name = "") = 0;
    virtual void serialize(DirectX::SimpleMath::Quaternion& val, const char* name = "") = 0;
    virtual void serialize(DirectX::SimpleMath::Color& val, const char* name = "") = 0;
    virtual void serialize(DirectX::SimpleMath::Matrix& val, const char* name = "") = 0;

    template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
    void serialize(T& val, const char* name = "")
    {
        uint32_t raw = static_cast<uint32_t>(val);
        serialize(raw, name);
        if (mode() == ArchiveMode::Input)
            val = static_cast<T>(raw);
    }

    virtual void serializeStringEnum(uint32_t& val, const char* name,
        const char* (*toString)(uint32_t),
        uint32_t (*fromString)(const char*))
    {
        serialize(val, name);
    }

    template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
    void serializeStringEnum(T& val, const char* name,
        const char* (*toString)(uint32_t),
        uint32_t (*fromString)(const char*))
    {
        uint32_t raw = static_cast<uint32_t>(val);
        serializeStringEnum(raw, name, toString, fromString);
        if (mode() == ArchiveMode::Input)
            val = static_cast<T>(raw);
    }

    virtual void beginObject(const char* name) {}
    virtual void beginObject() {}
    virtual void endObject() {}
    virtual void beginArray(uint32_t& count, const char* name) {}
    virtual void endArray() {}
};
