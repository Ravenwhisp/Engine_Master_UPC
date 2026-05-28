#pragma once
#include "IArchive.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include <vector>
#include <cstddef>

class BinaryArchive : public IArchive
{
public:
    BinaryArchive(ArchiveMode mode);
    BinaryArchive(const uint8_t* buffer, ArchiveMode mode);

    ArchiveMode mode() const override { return m_mode; }

    void serialize(uint8_t& val, const char* name = "") override;
    void serialize(uint32_t& val, const char* name = "") override;
    void serialize(uint64_t& val, const char* name = "") override;
    void serialize(float& val, const char* name = "") override;
    void serialize(bool& val, const char* name = "") override;
    void serialize(std::string& val, const char* name = "") override;
    void serializeRaw(void* data, size_t size, const char* name = "") override;

    void serialize(DirectX::SimpleMath::Vector3& val, const char* name = "") override;
    void serialize(DirectX::SimpleMath::Quaternion& val, const char* name = "") override;
    void serialize(DirectX::SimpleMath::Color& val, const char* name = "") override;
    void serialize(DirectX::SimpleMath::Matrix& val, const char* name = "") override;

    const uint8_t* data() const;
    size_t size() const;

private:
    size_t m_size = 0;
    size_t m_bytesWritten = 0;
    ArchiveMode m_mode;

    std::vector<uint8_t> m_outputBuffer;
    BinaryWriter m_writer;

    const uint8_t* m_inputData;
    BinaryReader m_reader;
};
