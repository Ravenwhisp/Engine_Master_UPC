#include "Globals.h"
#include "Metadata.h"

bool Metadata::toJson(rapidjson::Document& doc) const
{
    auto& alloc = doc.GetAllocator();

    doc.AddMember("uid", fileId, alloc);
    doc.AddMember("hash", rapidjson::Value(contentHash.c_str(), alloc), alloc);
    doc.AddMember("type", rapidjson::Value(static_cast<uint32_t>(type)), alloc);
    doc.AddMember("sourcePath", rapidjson::Value(sourcePath.string().c_str(), alloc), alloc);

    if (!m_dependencies.empty())
    {
        rapidjson::Value deps(rapidjson::kArrayType);
        for (const DependencyRecord& dep : m_dependencies)
        {
            rapidjson::Value entry(rapidjson::kObjectType);
            entry.AddMember("uid", dep.localId, alloc);
            entry.AddMember("type", rapidjson::Value(static_cast<uint32_t>(dep.type)), alloc);
            deps.PushBack(entry, alloc);
        }
        doc.AddMember("dependencies", deps, alloc);
    }
    return true;
}

bool Metadata::fromJson(const rapidjson::Value& root)
{
    if (!root.HasMember("uid") || !root["uid"].IsUint64())
    {
        DEBUG_ERROR("[Metadata] Missing or invalid 'uid'.");
        return false;
    }

    fileId = root["uid"].GetUint64();

    if (!root.HasMember("hash") || !root["hash"].IsString())
    {
        DEBUG_ERROR("[Metadata] Missing or invalid 'hash'.");
        return false;
    }

    contentHash = root["hash"].GetString();

    if (!root.HasMember("type") || !root["type"].IsNumber())
    {
        DEBUG_ERROR("[Metadata] Missing or invalid 'type'.");
        return false;
    }
    type = static_cast<AssetType>(root["type"].GetUint());

    if (root.HasMember("sourcePath") && root["sourcePath"].IsString())
    {
        sourcePath = root["sourcePath"].GetString();
    }

    m_dependencies.clear();
    if (root.HasMember("dependencies") && root["dependencies"].IsArray())
    {
        const auto& deps = root["dependencies"];
        m_dependencies.reserve(deps.Size());
        for (rapidjson::SizeType i = 0; i < deps.Size(); ++i)
        {
            const auto& entry = deps[i];
            if (!entry.HasMember("uid") || !entry["uid"].IsUint64())  continue;
            if (!entry.HasMember("type") || !entry["type"].IsNumber())  continue;

            DependencyRecord rec;
            rec.localId = entry["uid"].GetInt64();
            rec.type = static_cast<AssetType>(entry["type"].GetUint());
            m_dependencies.push_back(std::move(rec));
        }
    }

    return true;
}
