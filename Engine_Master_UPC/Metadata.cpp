#include "Globals.h"
#include "Metadata.h"

#include "MD5.h"
#include "ImportSettings.h"

void Metadata::serialize(IArchive& archive)
{
    archive.serialize(uid, "uid");

    {
        std::string hash = contentHash;
        archive.serialize(hash, "contentHash");
        if (archive.mode() == ArchiveMode::Input)
            contentHash = hash;
    }

    {
        uint32_t typeInt = static_cast<uint32_t>(type);
        archive.serialize(typeInt, "type");
        if (archive.mode() == ArchiveMode::Input)
            type = static_cast<AssetType>(typeInt);
    }

    {
        std::string pathStr = sourcePath.string();
        archive.serialize(pathStr, "sourcePath");
        if (archive.mode() == ArchiveMode::Input)
            sourcePath = pathStr;
    }

    archive.serialize(sourceFileSize, "sourceFileSize");

    if (archive.mode() == ArchiveMode::Output)
    {
        uint32_t depCount = static_cast<uint32_t>(m_dependencies.size());
        archive.serialize(depCount, "depCount");
        for (uint32_t i = 0; i < depCount; ++i)
        {
            std::string key = "dep_" + std::to_string(i);
            archive.beginObject(key.c_str());
            archive.serialize(m_dependencies[i].uid, "uid");
            {
                std::string hash = m_dependencies[i].contentHash;
                archive.serialize(hash, "contentHash");
            }
            {
                uint32_t depType = static_cast<uint32_t>(m_dependencies[i].type);
                archive.serialize(depType, "type");
                m_dependencies[i].type = static_cast<AssetType>(depType);
            }
            archive.serialize(m_dependencies[i].displayName, "displayName");
            archive.endObject();
        }
    }
    else
    {
        uint32_t depCount = 0;
        archive.serialize(depCount, "depCount");
        m_dependencies.resize(depCount);
        for (uint32_t i = 0; i < depCount; ++i)
        {
            std::string key = "dep_" + std::to_string(i);
            archive.beginObject(key.c_str());
            archive.serialize(m_dependencies[i].uid, "uid");
            {
                std::string hash;
                archive.serialize(hash, "contentHash");
                m_dependencies[i].contentHash = hash;
            }
            {
                uint32_t depType = 0;
                archive.serialize(depType, "type");
                m_dependencies[i].type = static_cast<AssetType>(depType);
            }
            archive.serialize(m_dependencies[i].displayName, "displayName");
            archive.endObject();
        }
    }

    if (archive.mode() == ArchiveMode::Output)
    {
        if (importSettings)
        {
            std::string typeName = importSettings->getTypeName();
            archive.serialize(typeName, "typeName");
            if (!typeName.empty())
            {
                archive.beginObject("importSettings");
                importSettings->serialize(archive);
                archive.endObject();
            }
        }
        else
        {
            std::string empty;
            archive.serialize(empty, "typeName");
        }
    }
    else
    {
        std::string typeName;
        archive.serialize(typeName, "typeName");
        if (!typeName.empty())
        {
            archive.beginObject("importSettings");
            importSettings = ImportSettings::CreateForType(type);
            if (importSettings)
                importSettings->serialize(archive);
            archive.endObject();
        }
    }
}
