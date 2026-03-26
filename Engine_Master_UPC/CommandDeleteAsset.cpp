#include "Globals.h"
#include "CommandDeleteAsset.h"

#include "FileIO.h"

CommandDeleteAsset::CommandDeleteAsset(const std::filesystem::path& metaPath)
    : m_metaPath(metaPath)
{
}

void CommandDeleteAsset::run()
{
    const std::filesystem::path sourcePath = m_metaPath.parent_path() / m_metaPath.stem();
    const bool deletedMeta = FileIO::remove(m_metaPath);
    const bool deletedSource = FileIO::remove(sourcePath);
    m_result = deletedMeta && deletedSource;
}

bool CommandDeleteAsset::getResult() const
{
    return m_result;
}
