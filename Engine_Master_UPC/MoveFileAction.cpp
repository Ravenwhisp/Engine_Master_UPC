#include "Globals.h"
#include "MoveFileAction.h"

#include "Application.h"
#include <ModuleFileSystem.h>

MoveFileAction::MoveFileAction(const std::filesystem::path& source,
    const std::filesystem::path& targetDir)
    : m_source(source)
    , m_targetDir(targetDir)
{
}

bool MoveFileAction::run()
{
    ModuleFileSystem* fs = app->getModuleFileSystem();

    const std::filesystem::path target = m_targetDir / m_source.filename();

    // Directories: move the whole tree in one call
    if (fs->isDirectory(m_source))
        return fs->move(m_source, target);

    // Files: move the .metadata sidecar and its source counterpart
    const std::filesystem::path sourceStem = m_source.parent_path() / m_source.stem();
    const std::filesystem::path targetStem = m_targetDir / m_source.stem();

    const bool movedMeta = fs->move(m_source, target);
    const bool movedSource = fs->move(sourceStem, targetStem);
    return movedMeta && movedSource;
}
