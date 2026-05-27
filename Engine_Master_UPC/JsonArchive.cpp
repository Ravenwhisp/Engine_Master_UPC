#include "Globals.h"
#include "JsonArchive.h"
#include "Metadata.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filereadstream.h>
#include <fstream>
#include <cstdio>

using namespace rapidjson;

JsonArchive::JsonArchive()
    : m_mode(ArchiveMode::Output)
    , m_currentInput(nullptr)
    , m_currentArrayIndex(0)
{
    m_doc.SetObject();
    m_valueStack.push_back(&m_doc);
    m_typeStack.push_back(Context::Object);
}

JsonArchive::JsonArchive(ArchiveMode mode)
    : m_mode(mode)
    , m_currentInput(nullptr)
    , m_currentArrayIndex(0)
{
    if (mode == ArchiveMode::Output)
    {
        m_doc.SetObject();
        m_valueStack.push_back(&m_doc);
        m_typeStack.push_back(Context::Object);
    }
}

bool JsonArchive::loadFile(const std::filesystem::path& path)
{
    const std::string pathStr = path.string();
    FILE* fp = std::fopen(pathStr.c_str(), "rb");
    if (!fp)
    {
        DEBUG_ERROR("[JsonArchive] Could not open '%s'.", pathStr.c_str());
        return false;
    }

    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    m_doc.ParseStream(is);
    std::fclose(fp);

    if (m_doc.HasParseError())
    {
        DEBUG_ERROR("[JsonArchive] JSON parse error in '%s'.", pathStr.c_str());
        return false;
    }

    m_currentInput = &m_doc;
    m_currentArrayIndex = 0;
    return true;
}

bool JsonArchive::saveFile(const std::filesystem::path& path) const
{
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    m_doc.Accept(writer);

    std::ofstream file(path);
    if (!file.is_open())
    {
        DEBUG_ERROR("[JsonArchive] Could not open '%s' for writing.", path.string().c_str());
        return false;
    }
    file << buffer.GetString();
    if (!file)
    {
        DEBUG_ERROR("[JsonArchive] Failed to write '%s'.", path.string().c_str());
        return false;
    }
    return true;
}

// --- IArchive implementation ---

void JsonArchive::serialize(uint8_t& val, const char* name)
{
    if (m_mode == ArchiveMode::Output)
    {
        if (m_valueStack.empty() || m_typeStack.back() != Context::Object) return;
        rapidjson::Value key(name, m_doc.GetAllocator());
        m_valueStack.back()->AddMember(key, rapidjson::Value(val), m_doc.GetAllocator());
    }
    else
    {
        if (!m_currentInput || !m_currentInput->HasMember(name)) return;
        const auto& v = (*m_currentInput)[name];
        if (v.IsUint()) val = static_cast<uint8_t>(v.GetUint());
    }
}

void JsonArchive::serialize(uint32_t& val, const char* name)
{
    if (m_mode == ArchiveMode::Output)
    {
        if (m_valueStack.empty() || m_typeStack.back() != Context::Object) return;
        rapidjson::Value key(name, m_doc.GetAllocator());
        m_valueStack.back()->AddMember(key, rapidjson::Value(val), m_doc.GetAllocator());
    }
    else
    {
        if (!m_currentInput || !m_currentInput->HasMember(name)) return;
        const auto& v = (*m_currentInput)[name];
        if (v.IsUint64()) val = static_cast<uint32_t>(v.GetUint64());
        else if (v.IsUint()) val = v.GetUint();
    }
}

void JsonArchive::serialize(uint64_t& val, const char* name)
{
    if (m_mode == ArchiveMode::Output)
    {
        if (m_valueStack.empty() || m_typeStack.back() != Context::Object) return;
        rapidjson::Value key(name, m_doc.GetAllocator());
        m_valueStack.back()->AddMember(key, rapidjson::Value(val), m_doc.GetAllocator());
    }
    else
    {
        if (!m_currentInput || !m_currentInput->HasMember(name)) return;
        const auto& v = (*m_currentInput)[name];
        if (v.IsUint64()) val = v.GetUint64();
    }
}

void JsonArchive::serialize(float& val, const char* name)
{
    if (m_mode == ArchiveMode::Output)
    {
        if (m_valueStack.empty() || m_typeStack.back() != Context::Object) return;
        rapidjson::Value key(name, m_doc.GetAllocator());
        m_valueStack.back()->AddMember(key, rapidjson::Value(val), m_doc.GetAllocator());
    }
    else
    {
        if (!m_currentInput || !m_currentInput->HasMember(name)) return;
        const auto& v = (*m_currentInput)[name];
        if (v.IsFloat()) val = v.GetFloat();
        else if (v.IsDouble()) val = static_cast<float>(v.GetDouble());
    }
}

void JsonArchive::serialize(bool& val, const char* name)
{
    if (m_mode == ArchiveMode::Output)
    {
        if (m_valueStack.empty() || m_typeStack.back() != Context::Object) return;
        rapidjson::Value key(name, m_doc.GetAllocator());
        m_valueStack.back()->AddMember(key, rapidjson::Value(val), m_doc.GetAllocator());
    }
    else
    {
        if (!m_currentInput || !m_currentInput->HasMember(name)) return;
        const auto& v = (*m_currentInput)[name];
        if (v.IsBool()) val = v.GetBool();
    }
}

void JsonArchive::serialize(std::string& val, const char* name)
{
    if (m_mode == ArchiveMode::Output)
    {
        if (m_valueStack.empty() || m_typeStack.back() != Context::Object) return;
        rapidjson::Value key(name, m_doc.GetAllocator());
        rapidjson::Value strVal;
        strVal.SetString(val.c_str(), static_cast<SizeType>(val.size()), m_doc.GetAllocator());
        m_valueStack.back()->AddMember(key, strVal, m_doc.GetAllocator());
    }
    else
    {
        if (!m_currentInput || !m_currentInput->HasMember(name)) return;
        const auto& v = (*m_currentInput)[name];
        if (v.IsString()) val = v.GetString();
    }
}

void JsonArchive::serializeRaw(void* data, size_t size, const char* /*name*/)
{
    // JSON does not support raw binary data.
    DEBUG_ERROR("[JsonArchive] serializeRaw not supported for JSON.");
}

// --- JSON-specific methods ---

void JsonArchive::beginObject(const char* name)
{
    if (m_mode == ArchiveMode::Output)
    {
        rapidjson::Value obj(rapidjson::kObjectType);
        rapidjson::Value key(name, m_doc.GetAllocator());
        m_valueStack.back()->AddMember(key, obj, m_doc.GetAllocator());
        m_valueStack.push_back(&(*m_valueStack.back())[name]);
        m_typeStack.push_back(Context::Object);
    }
    else
    {
        if (m_currentInput && m_currentInput->HasMember(name))
        {
            m_currentInput = &(*m_currentInput)[name];
            m_currentArrayIndex = 0;
        }
    }
}

void JsonArchive::endObject()
{
    if (m_mode == ArchiveMode::Output)
    {
        if (m_valueStack.size() > 1)
        {
            m_valueStack.pop_back();
            m_typeStack.pop_back();
        }
    }
    else
    {
        // We don't maintain a full input stack for simplicity.
        // Objects are navigated by name via beginObject().
        // For flat Metadata, this is acceptable.
        m_currentArrayIndex = 0;
    }
}

void JsonArchive::beginArray(const char* name)
{
    if (m_mode == ArchiveMode::Output)
    {
        rapidjson::Value arr(rapidjson::kArrayType);
        rapidjson::Value key(name, m_doc.GetAllocator());
        m_valueStack.back()->AddMember(key, arr, m_doc.GetAllocator());
        m_valueStack.push_back(&(*m_valueStack.back())[name]);
        m_typeStack.push_back(Context::Array);
    }
    else
    {
        if (m_currentInput && m_currentInput->HasMember(name))
        {
            m_currentInput = &(*m_currentInput)[name];
            m_currentArrayIndex = 0;
        }
    }
}

void JsonArchive::endArray()
{
    if (m_mode == ArchiveMode::Output)
    {
        if (m_valueStack.size() > 1)
        {
            m_valueStack.pop_back();
            m_typeStack.pop_back();
        }
    }
    else
    {
        m_currentArrayIndex = 0;
    }
}

size_t JsonArchive::arraySize() const
{
    if (m_currentInput && m_currentInput->IsArray())
    {
        return m_currentInput->Size();
    }
    return 0;
}

bool JsonArchive::hasKey(const char* name) const
{
    return m_currentInput && m_currentInput->HasMember(name);
}

bool JsonArchive::read(const char* key, uint64_t& val) const
{
    if (!m_currentInput || !m_currentInput->HasMember(key)) return false;
    const auto& v = (*m_currentInput)[key];
    if (v.IsUint64()) { val = v.GetUint64(); return true; }
    if (v.IsUint()) { val = v.GetUint(); return true; }
    return false;
}

bool JsonArchive::read(const char* key, std::string& val) const
{
    if (!m_currentInput || !m_currentInput->HasMember(key)) return false;
    const auto& v = (*m_currentInput)[key];
    if (v.IsString()) { val = v.GetString(); return true; }
    return false;
}

