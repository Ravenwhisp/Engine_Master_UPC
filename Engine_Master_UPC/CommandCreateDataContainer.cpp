#include "Globals.h"
#include "CommandCreateDataContainer.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "DataContainerFactory.h"
#include "UID.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <fstream>

CommandCreateDataContainer::CommandCreateDataContainer(
    const std::filesystem::path& targetDir,
    const std::string& typeName,
    const std::string& assetName,
    const std::string& extension)
    : m_targetDir(targetDir)
    , m_typeName(typeName)
    , m_assetName(assetName)
    , m_extension(extension)
{
}

void CommandCreateDataContainer::run()
{
    std::filesystem::path filePath = m_targetDir / (m_assetName + m_extension);

    if (std::filesystem::exists(filePath))
    {
        DEBUG_WARN("[CommandCreateDataContainer] File '%s' already exists.", filePath.string().c_str());
        return;
    }

    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    doc.AddMember("_typeName", rapidjson::Value(m_typeName.c_str(), allocator), allocator);

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
