#include "Globals.h"
#include "CommandCutItem.h"

#include "FileDialogClipboard.h"

CommandCutItem::CommandCutItem(FileDialogClipboard& clipboard,
    const std::filesystem::path& itemPath)
    : m_clipboard(clipboard)
    , m_itemPath(itemPath)
{
}

void CommandCutItem::run()
{
    m_clipboard.command = FileDialogCommand::Move;
    m_clipboard.fileToManage = m_itemPath;
}
