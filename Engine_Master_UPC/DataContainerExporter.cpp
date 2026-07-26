#include "Globals.h"
#include "DataContainerExporter.h"

#include "AssetsDictionary.h"
#include "Extensions.h"
#include "Application.h"
#include "ModuleAssets.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include "rapidjson/filereadstream.h"

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

bool DataContainerExporter::exportToJson(const fs::path& outputPath)
{
    rapidjson::Document root;
    root.SetObject();
    rapidjson::Document::AllocatorType& allocator = root.GetAllocator();

    fs::path assetsRoot = ASSETS_FOLDER;
    if (!assetsRoot.is_absolute())
    {
        assetsRoot = fs::absolute(assetsRoot);
    }

    if (!fs::exists(assetsRoot))
    {
        DEBUG_ERROR("[DataContainerExporter] Assets folder '%s' not found.", assetsRoot.string().c_str());
        return false;
    }

    // Scan recursively for .datacontainer files
    for (const auto& entry : fs::recursive_directory_iterator(assetsRoot))
    {
        if (!entry.is_regular_file())
            continue;

        const fs::path& filePath = entry.path();
        if (filePath.extension().string() != DATA_CONTAINER_EXTENSION)
            continue;

        // Read the .datacontainer file
        FILE* fp = std::fopen(filePath.string().c_str(), "rb");
        if (!fp)
        {
            DEBUG_WARN("[DataContainerExporter] Could not open '%s'.", filePath.string().c_str());
            continue;
        }

        char readBuffer[65536];
        rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
        rapidjson::Document doc;
        doc.ParseStream(is);
        std::fclose(fp);

        if (doc.HasParseError() || !doc.IsObject())
        {
            DEBUG_WARN("[DataContainerExporter] Parse error in '%s'.", filePath.string().c_str());
            continue;
        }

        // Use filename stem as key
        std::string key = filePath.stem().string();
        rapidjson::Value keyVal(key.c_str(), allocator);

        // Deep-copy the document's members into a new value
        rapidjson::Value obj(rapidjson::kObjectType);
        for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it)
        {
            rapidjson::Value name(it->name.GetString(), allocator);
            rapidjson::Value value;
            value.CopyFrom(it->value, allocator);
            obj.AddMember(name, value, allocator);
        }

        root.AddMember(keyVal, obj, allocator);

        DEBUG_LOG("[DataContainerExporter] Exported '%s'.", key.c_str());
    }

    if (root.MemberCount() == 0)
    {
        DEBUG_WARN("[DataContainerExporter] No .datacontainer files found to export.");
        return false;
    }

    // Write combined JSON
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    root.Accept(writer);

    std::ofstream file(outputPath);
    if (!file.is_open())
    {
        DEBUG_ERROR("[DataContainerExporter] Could not write '%s'.", outputPath.string().c_str());
        return false;
    }

    file << buffer.GetString();
    file.close();

    DEBUG_LOG("[DataContainerExporter] Exported %llu data assets to '%s'.",
        static_cast<unsigned long long>(root.MemberCount()), outputPath.string().c_str());
    return true;
}

bool DataContainerExporter::importFromJson(const fs::path& inputPath)
{
    FILE* fp = std::fopen(inputPath.string().c_str(), "rb");
    if (!fp)
    {
        DEBUG_ERROR("[DataContainerExporter] Could not open '%s'.", inputPath.string().c_str());
        return false;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    rapidjson::Document root;
    root.ParseStream(is);
    std::fclose(fp);

    if (root.HasParseError() || !root.IsObject())
    {
        DEBUG_ERROR("[DataContainerExporter] Parse error in '%s'.", inputPath.string().c_str());
        return false;
    }

    fs::path assetsRoot = ASSETS_FOLDER;
    if (!assetsRoot.is_absolute())
    {
        assetsRoot = fs::absolute(assetsRoot);
    }

    if (!fs::exists(assetsRoot))
    {
        DEBUG_ERROR("[DataContainerExporter] Assets folder '%s' not found.", assetsRoot.string().c_str());
        return false;
    }

    size_t importedCount = 0;

    for (auto it = root.MemberBegin(); it != root.MemberEnd(); ++it)
    {
        if (!it->value.IsObject())
        {
            DEBUG_WARN("[DataContainerExporter] Skipping '%s': value is not an object.", it->name.GetString());
            continue;
        }

        std::string name = it->name.GetString();
        fs::path filePath = assetsRoot / (name + DATA_CONTAINER_EXTENSION);

        // Serialize this entry back to JSON
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        it->value.Accept(writer);

        std::ofstream file(filePath);
        if (!file.is_open())
        {
            DEBUG_ERROR("[DataContainerExporter] Could not write '%s'.", filePath.string().c_str());
            continue;
        }

        file << buffer.GetString();
        file.close();

        DEBUG_LOG("[DataContainerExporter] Imported '%s'.", name.c_str());
        ++importedCount;
    }

    // Refresh the asset system to pick up new/changed files
    app->getModuleAssets()->refresh();

    DEBUG_LOG("[DataContainerExporter] Imported %zu data assets from '%s'.",
        importedCount, inputPath.string().c_str());
    return importedCount > 0;
}
