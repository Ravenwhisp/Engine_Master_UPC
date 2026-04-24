#include "Globals.h"
#include "FileDialog.h"
#include "Application.h"

std::optional<std::filesystem::path> runDialog(bool isSave, const char* filterSpec, const char* defaultExt, const char* title, const char* initialDir)
{
    char buf[MAX_PATH] = {};

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = app->getWindowHandle();
    ofn.lpstrFilter = filterSpec;
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = defaultExt;
    ofn.lpstrTitle = title;
    ofn.lpstrInitialDir = initialDir;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    BOOL ok = isSave ? GetSaveFileNameA(&ofn) : GetOpenFileNameA(&ofn);

    if (!ok)
    {
        return std::nullopt;
    }

    return std::filesystem::path(buf);
}

std::optional<std::filesystem::path> saveAs(const char* filterSpec, const char* defaultExtension, const char* title, const char* initialDir)
{
    return runDialog(true, filterSpec, defaultExtension, title, initialDir);
}

std::optional<std::filesystem::path> open(const char* filterSpec, const char* title, const char* initialDir)
{
    return runDialog(false, filterSpec, nullptr, title, initialDir);
}
