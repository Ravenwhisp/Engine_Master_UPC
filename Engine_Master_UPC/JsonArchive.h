#pragma once
#include "IArchive.h"
#include "UID.h"
#include "MD5Fwd.h"
#include "AssetType.h"
#include "ImportSettings.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <stack>

struct DependencyRecord;

class JsonArchive : public IArchive
{
public:
    JsonArchive();
    explicit JsonArchive(ArchiveMode mode);

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

    // File I/O
    bool loadFile(const std::filesystem::path& path);
    bool saveFile(const std::filesystem::path& path) const;

    // Named object/array scopes (JSON-specific)
    void beginObject(const char* name);
    void endObject();
    void beginArray(const char* name);
    void endArray();
    size_t arraySize() const;

    // Extract/copy the built document value for use in rapidjson context
    rapidjson::Value extractValue(rapidjson::Document::AllocatorType& allocator) const;
    void setValue(const rapidjson::Value& val);

    // Direct key access for reading
    bool hasKey(const char* name) const;
    bool read(const char* key, uint64_t& val) const;
    bool read(const char* key, std::string& val) const;

private:
    struct Context
    {
        enum Type { Object, Array };
        Type type;
        rapidjson::Value* value;       // for writing
        const rapidjson::Value* input; // for reading
        int nextIndex;                 // for array reading
    };

    void pushWriteObject(const char* name, rapidjson::Value* val);
    void popWrite();

    ArchiveMode m_mode;

    rapidjson::Document m_doc;

    // Write state
    std::vector<rapidjson::Value*> m_valueStack;
    std::vector<Context::Type> m_typeStack;

    // Read state
    const rapidjson::Value* m_currentInput;
    std::vector<const rapidjson::Value*> m_inputStack;
    int m_currentArrayIndex;
};
