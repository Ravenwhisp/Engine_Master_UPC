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
    if (m_mode == ArchiveMode::Output)
        m_writer.u8(val);
    else
        val = m_reader.u8();
}

void BinaryArchive::serialize(uint32_t& val, const char* /*name*/)
{
    if (m_mode == ArchiveMode::Output)
        m_writer.u32(val);
    else
        val = m_reader.u32();
}

void BinaryArchive::serialize(uint64_t& val, const char* /*name*/)
{
    if (m_mode == ArchiveMode::Output)
        m_writer.u64(val);
    else
        val = m_reader.u64();
}

void BinaryArchive::serialize(float& val, const char* /*name*/)
{
    serializeRaw(&val, sizeof(val));
}

void BinaryArchive::serialize(bool& val, const char* /*name*/)
{
    uint8_t b = val ? 1 : 0;
    serialize(b);
    if (m_mode == ArchiveMode::Input) val = (b != 0);
}

void BinaryArchive::serialize(std::string& val, const char* /*name*/)
{
    if (m_mode == ArchiveMode::Output)
        m_writer.string(val);
    else
        val = m_reader.string();
}

void BinaryArchive::serializeRaw(void* data, size_t size, const char* /*name*/)
{
    if (m_mode == ArchiveMode::Output)
        m_writer.bytes(data, size);
    else
        m_reader.bytes(data, size);
}

const uint8_t* BinaryArchive::data() const
{
    return m_outputBuffer.data();
}

size_t BinaryArchive::size() const
{
    if (m_mode == ArchiveMode::Output)
    {
        return m_writer.offset();
    }
    return 0;
}
