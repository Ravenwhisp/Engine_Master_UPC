#pragma once
#include "UID.h"
#include "MD5Fwd.h"
#include <filesystem>

constexpr const char* ASSETS_FOLDER = "Assets/";
constexpr const char* LIBRARY_FOLDER = "Library/";

constexpr const char* ASSET_EXTENSION = ".asset";
constexpr const char* METADATA_EXTENSION = ".metadata";

inline std::filesystem::path buildLibraryPath(const MD5Hash& hash)
{
    return std::filesystem::path(LIBRARY_FOLDER) / (hash + ASSET_EXTENSION);
}