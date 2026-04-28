#pragma once
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>

#include <sstream>
#include <streambuf>
#include <cstring>
#include <cstdint>


namespace cereal
{
    template<class Archive> void serialize(Archive& ar, Vector2& v) { ar(v.x, v.y); }
    template<class Archive> void serialize(Archive& ar, Vector3& v) { ar(v.x, v.y, v.z); }
    template<class Archive> void serialize(Archive& ar, Vector4& v) { ar(v.x, v.y, v.z, v.w); }
    template<class Archive> void serialize(Archive& ar, Color& c) { ar(c.x, c.y, c.z, c.w); }
    template<class Archive> void serialize(Archive& ar, Quaternion& q) { ar(q.x, q.y, q.z, q.w); }
    template<class Archive>
    void serialize(Archive& ar, DirectX::SimpleMath::Matrix& m)
    {
        ar(cereal::binary_data(m.m, sizeof(m.m)));
    }
}

namespace CerealUtils
{

    template<typename T>
    uint64_t saveTo(const T& obj, uint8_t** outBuffer)
    {
        std::ostringstream oss;
        {
            cereal::BinaryOutputArchive archive(oss);
            archive(obj);
        }

        const std::string payload = oss.str();
        const uint64_t    payloadSize = static_cast<uint64_t>(payload.size());
        const uint64_t    totalSize = sizeof(uint64_t) + payloadSize;

        uint8_t* buf = new uint8_t[totalSize];
        std::memcpy(buf, &payloadSize, sizeof(uint64_t));
        std::memcpy(buf + sizeof(uint64_t), payload.data(), payloadSize);

        *outBuffer = buf;
        return totalSize;
    }

    template<typename T>
    void loadFrom(const uint8_t* buffer, T& obj)
    {
        uint64_t payloadSize = 0;
        std::memcpy(&payloadSize, buffer, sizeof(uint64_t));

        // Minimal read-only streambuf that wraps a raw pointer.
        // Defined here so it stays an implementation detail of this header.
        struct BoundedMemBuf : std::streambuf
        {
            BoundedMemBuf(const char* data, std::size_t size)
            {
                char* p = const_cast<char*>(data);
                setg(p, p, p + size);
            }
        };

        const char* payload = reinterpret_cast<const char*>(buffer + sizeof(uint64_t));
        BoundedMemBuf memBuf(payload, static_cast<std::size_t>(payloadSize));
        std::istream  is(&memBuf);

        cereal::BinaryInputArchive archive(is);
        archive(obj);
    }

} // namespace CerealUtils