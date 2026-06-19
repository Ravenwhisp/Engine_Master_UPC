#include "Globals.h"
#include "BinaryArchive.h"
#include <cstring>

BinaryArchive::BinaryArchive(ArchiveMode mode)
    : m_mode(mode)
    , m_outputBuffer()
    , m_writer(nullptr)
    , m_inputData(nullptr)
    , m_reader(nullptr)
{
    if (mode == ArchiveMode::Output)
    {
        m_outputBuffer.resize(1024 * 1024);
        m_writer = BinaryWriter(m_outputBuffer.data());
    }
}

BinaryArchive::BinaryArchive(const uint8_t* buffer, ArchiveMode mode)
    : m_outputBuffer()
    , m_writer(nullptr)
    , m_inputData(buffer)
    , m_reader(buffer)
{
    if (mode != ArchiveMode::Input)
    {
        DEBUG_ERROR("[BinaryArchive] Buffer constructor requires Input mode.");
    }
    m_mode = ArchiveMode::Input;
}

void BinaryArchive::serialize(uint8_t& val, const char* /*name*/)
{
    serializeRaw(&val, sizeof(val));
}

void BinaryArchive::serialize(uint32_t& val, const char* /*name*/)
{
    serializeRaw(&val, sizeof(val));
}

void BinaryArchive::serialize(uint64_t& val, const char* /*name*/)
{
    serializeRaw(&val, sizeof(val));
}

void BinaryArchive::serialize(float& val, const char* /*name*/)
{
    serializeRaw(&val, sizeof(val));
}

void BinaryArchive::serialize(bool& val, const char* /*name*/)
{
    uint8_t b = val ? 1 : 0;
    serializeRaw(&b, sizeof(b));
    if (m_mode == ArchiveMode::Input) val = (b != 0);
}

void BinaryArchive::serialize(std::string& val, const char* /*name*/)
{
    if (m_mode == ArchiveMode::Output)
    {
        uint32_t len = static_cast<uint32_t>(val.size());
        serializeRaw(&len, sizeof(len));
        serializeRaw(val.data(), len);
    }
    else
    {
        uint32_t len = 0;
        serializeRaw(&len, sizeof(len));
        val.resize(len);
        serializeRaw(val.data(), len);
    }
}

void BinaryArchive::serializeRaw(void* data, size_t size, const char* /*name*/)
{
    if (m_mode == ArchiveMode::Output)
    {
        if (m_bytesWritten + size > m_outputBuffer.size())
        {
            size_t newSize = m_outputBuffer.size() * 2;
            if (newSize < m_bytesWritten + size)
                newSize = m_bytesWritten + size;
            m_outputBuffer.resize(newSize);
            m_writer = BinaryWriter(m_outputBuffer.data() + m_bytesWritten);
        }
        m_writer.bytes(data, size);
        m_bytesWritten += size;

    }
    else
        m_reader.bytes(data, size);

}

void BinaryArchive::beginArray(uint32_t& count, const char* /*name*/)
{
    serializeRaw(&count, sizeof(count));
}

void BinaryArchive::serialize(DirectX::SimpleMath::Vector3& val, const char* /*name*/)
{
    serializeRaw(&val, sizeof(val));
}

void BinaryArchive::serialize(DirectX::SimpleMath::Quaternion& val, const char* /*name*/)
{
    serializeRaw(&val, sizeof(val));
}

void BinaryArchive::serialize(DirectX::SimpleMath::Color& val, const char* /*name*/)
{
    serializeRaw(&val, sizeof(val));
}

void BinaryArchive::serialize(DirectX::SimpleMath::Matrix& val, const char* /*name*/)
{
    serializeRaw(&val, sizeof(val));
}

const uint8_t* BinaryArchive::data() const
{
    return m_outputBuffer.data();
}

size_t BinaryArchive::size() const
{
    if (m_mode == ArchiveMode::Output)
    {
        return m_bytesWritten;
    }
    return 0;
}
