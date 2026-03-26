#include "Globals.h"
#include "CommandMoveFile.h"

#include "Application.h"
#include <ModuleFileSystem.h>

CommandMoveFile::CommandMoveFile(const std::filesystem::path& source,
    const std::filesystem::path& targetDir)
    : m_source(source)
    , m_targetDir(targetDir)
{
}

void CommandMoveFile::run()
{
    ModuleFileSystem* fs = app->getModuleFileSystem();

    const std::filesystem::path target = m_targetDir / m_source.filename();

    if (fs->isDirectory(m_source))
    {
        m_result = fs->move(m_source, target);
        return;
    }

    const std::filesystem::path sourceStem = m_source.parent_path() / m_source.stem();
    const std::filesystem::path targetStem = m_targetDir / m_source.stem();

    const bool movedMeta = fs->move(m_source, target);
    const bool movedSource = fs->move(sourceStem, targetStem);
    m_result = movedMeta && movedSource;
}

bool CommandMoveFile::getResult() const
{
    return m_result;
}
