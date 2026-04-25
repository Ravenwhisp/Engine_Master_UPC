#pragma once
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>

#include <sstream>
#include <streambuf>
#include <cstring>
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// CerealUtils — drop-in helpers for Importer saveTyped / loadTyped.
//
// Binary layout on disk:
//   [ uint64_t payloadSize | <payloadSize> bytes of cereal::BinaryOutputArchive ]
//
// The size prefix lets loadFrom() reconstruct a bounded input stream from a
// raw pointer without changing the Importer::load(const uint8_t*, Asset*)
// interface.
// ─────────────────────────────────────────────────────────────────────────────

namespace CerealUtils
{
    // saveTo
    // Serializes `obj` via cereal and allocates *outBuffer (caller takes
    // ownership). Returns total byte count including the 8-byte prefix.
    //
    // Usage inside saveTyped:
    //   return CerealUtils::saveTo(*source, outBuffer);
    template<typename T>
    uint64_t saveTo(const T& obj, uint8_t** outBuffer)
    {
        std::ostringstream oss(std::ios::binary);
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

    // loadFrom
    // Reads the size prefix from `buffer`, then deserializes into `obj`.
    // Zero-copy: no heap allocation, no data duplication.
    //
    // Usage inside loadTyped:
    //   CerealUtils::loadFrom(buffer, *texture);
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