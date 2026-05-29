#include "Globals.h"
#include "Metadata.h"

#include "MD5.h"
#include "ImportSettings.h"

void Metadata::serialize(IArchive& archive)
{
    archive.serialize(uid, "uid");

    {
        std::string hash = archive.mode() == ArchiveMode::Output ? contentHash : "";
        archive.serialize(hash, "contentHash");
        if (archive.mode() == ArchiveMode::Input) contentHash = hash;
    }

    {
        uint32_t t = static_cast<uint32_t>(type);
        archive.serialize(t, "type");
        if (archive.mode() == ArchiveMode::Input) type = static_cast<AssetType>(t);
    }

    {
        std::string path = archive.mode() == ArchiveMode::Output ? sourcePath.string() : "";
        archive.serialize(path, "sourcePath");
        if (archive.mode() == ArchiveMode::Input) sourcePath = path;
    }

    archive.serialize(sourceFileSize, "sourceFileSize");

    {
        uint32_t depCount = static_cast<uint32_t>(m_dependencies.size());
        archive.serialize(depCount, "depCount");
        if (archive.mode() == ArchiveMode::Input) m_dependencies.resize(depCount);

        for (uint32_t i = 0; i < depCount; ++i)
        {
            std::string key = "dep_" + std::to_string(i);
            archive.beginObject(key.c_str());

            auto& dep = m_dependencies[i];
            archive.serialize(dep.uid, "uid");

            std::string hash = archive.mode() == ArchiveMode::Output ? dep.contentHash : "";
            archive.serialize(hash, "contentHash");
            if (archive.mode() == ArchiveMode::Input) dep.contentHash = hash;

            uint32_t depType = static_cast<uint32_t>(dep.type);
            archive.serialize(depType, "type");
            dep.type = static_cast<AssetType>(depType);

            archive.serialize(dep.displayName, "displayName");

            archive.endObject();
        }
    }

    {
        std::string typeName;
        if (archive.mode() == ArchiveMode::Output && importSettings)
            typeName = importSettings->getTypeName();
        archive.serialize(typeName, "typeName");

        if (!typeName.empty())
        {
            archive.beginObject("importSettings");
            if (archive.mode() == ArchiveMode::Input)
                importSettings = ImportSettings::CreateForType(type);
            if (importSettings)
                importSettings->serialize(archive);
            archive.endObject();
        }
    }
}
