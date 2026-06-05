#include "Globals.h"
#include "CommandCreateDataContainer.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "GenericTypeFactory.h"
#include "DataContainer.h"
#include "Extensions.h"
#include "UID.h"
#include "JsonArchive.h"
#include "FieldUtils.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <fstream>

CommandCreateDataContainer::CommandCreateDataContainer(
    const std::filesystem::path& targetDir,
    const std::string& typeName,
    const std::string& assetName)
    : m_targetDir(targetDir)
    , m_typeName(typeName)
    , m_assetName(assetName)
{
}

void CommandCreateDataContainer::run()
{
    std::filesystem::path filePath = m_targetDir / (m_assetName + DATA_CONTAINER_EXTENSION);

    if (std::filesystem::exists(filePath))
    {
        int suffix = 2;
        while (true)
        {
            std::string numberedName = m_assetName + "_" + std::to_string(suffix);
            filePath = m_targetDir / (numberedName + DATA_CONTAINER_EXTENSION);
            if (!std::filesystem::exists(filePath))
            {
                break;
            }
            ++suffix;
        }
    }

    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    doc.AddMember("_typeName", rapidjson::Value(m_typeName.c_str(), allocator), allocator);

    AssetReference tempRef(INVALID_UID);
    auto instance = DataContainerFactory::create(m_typeName, tempRef);
    if (instance)
    {
        JsonArchive archive(ArchiveMode::Output);
        instance->serialize(archive);
        rapidjson::Value instanceJson = archive.extractValue(allocator);

        if (instanceJson.IsObject())
        {
            for (auto it = instanceJson.MemberBegin(); it != instanceJson.MemberEnd(); ++it)
            {
                if (!doc.HasMember(it->name.GetString()))
                {
                    doc.AddMember(
                        rapidjson::Value(it->name, allocator),
                        rapidjson::Value(it->value, allocator),
                        allocator
                    );
                }
            }
        }
    }

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream file(filePath);
    if (!file.is_open())
    {
        DEBUG_ERROR("[CommandCreateDataContainer] Could not create '%s'.", filePath.string().c_str());
        return;
    }

    file << buffer.GetString();
    file.close();

    app->getModuleAssets()->refresh();
}
