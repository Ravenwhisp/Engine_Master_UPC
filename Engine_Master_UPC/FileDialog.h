#pragma once
#include "FileDialog.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <optional>
#include <filesystem>
#pragma comment(lib, "comdlg32.lib")

static std::optional<std::filesystem::path> runDialog(bool isSave, const char* filterSpec, const char* defaultExt, const char* title, const char* initialDir);

std::optional<std::filesystem::path> saveAs(const char* filterSpec, const char* defaultExtension, const char* title, const char* initialDir);

std::optional<std::filesystem::path> open(const char* filterSpec, const char* title, const char* initialDir);