#include "Globals.h"
#include "CutItemAction.h"

#include "FileDialogClipboard.h"

CutItemAction::CutItemAction(FileDialogClipboard& clipboard,
    const std::filesystem::path& itemPath)
    : m_clipboard(clipboard)
    , m_itemPath(itemPath)
{
}

void CutItemAction::run()
{
    m_clipboard.command = FileDialogCommand::Move;
    m_clipboard.fileToManage = m_itemPath;
}