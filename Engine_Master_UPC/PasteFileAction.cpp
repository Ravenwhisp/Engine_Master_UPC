#include "Globals.h"
#include "PasteFileAction.h"

#include "Application.h"
#include "ModuleAssets.h"

#include "FileDialogClipboard.h"
#include <MoveFileAction.h>

PasteFileAction::PasteFileAction(FileDialogClipboard& clipboard,
    const std::filesystem::path& targetDir)
    : m_clipboard(clipboard)
    , m_targetDir(targetDir)
{
}

void PasteFileAction::run()
{
    if (!m_clipboard.hasPending() || !std::filesystem::exists(m_clipboard.fileToManage) || !std::filesystem::exists(m_targetDir))
    {
        return;
    }

    if (m_clipboard.command == FileDialogCommand::Move)
    {
        MoveFileAction(m_clipboard.fileToManage, m_targetDir).run();
    }

    m_clipboard.clear();
    app->getModuleAssets()->refresh();
}
