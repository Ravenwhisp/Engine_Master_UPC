#include "Globals.h"
#include "CommandMoveFile.h"

#include "Application.h"
#include <FileIO.h>

CommandMoveFile::CommandMoveFile(const std::filesystem::path& source,
    const std::filesystem::path& targetDir)
    : m_source(source)
    , m_targetDir(targetDir)
{
}

void CommandMoveFile::run()
{
    const std::filesystem::path target = m_targetDir / m_source.filename();

    if (FileIO::isDirectory(m_source))
    {
        m_result = FileIO::move(m_source, target);
        return;
    }

    const std::filesystem::path sourceStem = m_source.parent_path() / m_source.stem();
    const std::filesystem::path targetStem = m_targetDir / m_source.stem();

    const bool movedMeta = FileIO::move(m_source, target);
    const bool movedSource = FileIO::move(sourceStem, targetStem);
    m_result = movedMeta && movedSource;
}

bool CommandMoveFile::getResult() const
{
    return m_result;
}
