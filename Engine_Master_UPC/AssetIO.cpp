#include "Globals.h"
#include "AssetIO.h"

#include "MD5.h"
#include "AssetType.h"
#include "ImportSettings.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/filereadstream.h>
#include <fstream>
#include <cstdio>

using namespace rapidjson;

namespace AssetIO
{

bool saveMetaFile(const Metadata& meta, const std::filesystem::path& metaPath)
{
    Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("uid", meta.uid, alloc);
    doc.AddMember("contentHash", Value(meta.contentHash.c_str(), alloc), alloc);
    doc.AddMember("type", Value(static_cast<uint32_t>(meta.type)), alloc);
    doc.AddMember("sourcePath", Value(meta.sourcePath.string().c_str(), alloc), alloc);
    doc.AddMember("sourceFileSize", Value(meta.sourceFileSize), alloc);

    if (!meta.m_dependencies.empty())
    {
        Value deps(kArrayType);
        for (const DependencyRecord& dep : meta.m_dependencies)
        {
            Value entry(kObjectType);
            entry.AddMember("uid", dep.uid, alloc);
            entry.AddMember("contentHash", Value(dep.contentHash.c_str(), alloc), alloc);
            entry.AddMember("type", Value(static_cast<uint32_t>(dep.type)), alloc);
            if (!dep.displayName.empty())
                entry.AddMember("displayName", Value(dep.displayName.c_str(), alloc), alloc);
            deps.PushBack(entry, alloc);
        }
        doc.AddMember("dependencies", deps, alloc);
    }

    if (meta.importSettings)
    {
        Value settingsObj(kObjectType);
        settingsObj.AddMember("typeName", Value(meta.importSettings->getTypeName(), alloc), alloc);
        meta.importSettings->save(settingsObj, alloc);
        doc.AddMember("importSettings", settingsObj, alloc);
    }

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream file(metaPath);
    if (!file.is_open())
    {
        DEBUG_ERROR("[AssetIO] Could not open '%s' for writing.", metaPath.string().c_str());
        return false;
    }
    file << buffer.GetString();
    if (!file)
    {
        DEBUG_ERROR("[AssetIO] Failed to write '%s'.", metaPath.string().c_str());
        return false;
    }
    return true;
}

bool loadMetaFile(const std::filesystem::path& metaPath, Metadata& outMeta)
{
    const std::string pathStr = metaPath.string();
    FILE* fp = std::fopen(pathStr.c_str(), "rb");
    if (!fp)
    {
        DEBUG_ERROR("[AssetIO] Could not open '%s'.", pathStr.c_str());
        return false;
    }

    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    Document doc;
    doc.ParseStream(is);
    std::fclose(fp);

    if (doc.HasParseError())
    {
        DEBUG_ERROR("[AssetIO] JSON parse error in '%s'.", pathStr.c_str());
        return false;
    }

    if (doc.HasMember("uid") && doc["uid"].IsUint64())
    {
        outMeta.uid = doc["uid"].GetUint64();
    }
    else
    {
        DEBUG_ERROR("[AssetIO] Missing 'uid' in '%s'.", pathStr.c_str());
        return false;
    }

    outMeta.contentHash = (doc.HasMember("contentHash") && doc["contentHash"].IsString())
        ? doc["contentHash"].GetString()
        : INVALID_ASSET_ID;

    if (doc.HasMember("type") && doc["type"].IsNumber())
    {
        outMeta.type = static_cast<AssetType>(doc["type"].GetUint());
    }
    else
    {
        DEBUG_ERROR("[AssetIO] Missing 'type' in '%s'.", pathStr.c_str());
        return false;
    }

    if (doc.HasMember("sourcePath") && doc["sourcePath"].IsString())
        outMeta.sourcePath = doc["sourcePath"].GetString();

    if (doc.HasMember("sourceFileSize") && doc["sourceFileSize"].IsUint64())
        outMeta.sourceFileSize = doc["sourceFileSize"].GetUint64();

    outMeta.m_dependencies.clear();
    if (doc.HasMember("dependencies") && doc["dependencies"].IsArray())
    {
        const auto& deps = doc["dependencies"];
        outMeta.m_dependencies.reserve(deps.Size());
        for (SizeType i = 0; i < deps.Size(); ++i)
        {
            const auto& entry = deps[i];
            if (!entry.HasMember("uid") || !entry["uid"].IsUint64())  continue;
            if (!entry.HasMember("type") || !entry["type"].IsNumber()) continue;

            DependencyRecord rec;
            rec.uid = entry["uid"].GetUint64();
            rec.type = static_cast<AssetType>(entry["type"].GetUint());
            if (entry.HasMember("contentHash") && entry["contentHash"].IsString())
                rec.contentHash = entry["contentHash"].GetString();
            if (entry.HasMember("displayName") && entry["displayName"].IsString())
                rec.displayName = entry["displayName"].GetString();

            outMeta.m_dependencies.push_back(std::move(rec));
        }
    }

    if (doc.HasMember("importSettings") && doc["importSettings"].IsObject())
    {
        outMeta.importSettings = ImportSettings::CreateForType(outMeta.type);
        if (outMeta.importSettings)
        {
            outMeta.importSettings->load(doc["importSettings"]);
        }
    }

    return true;
}

} // namespace AssetIO
