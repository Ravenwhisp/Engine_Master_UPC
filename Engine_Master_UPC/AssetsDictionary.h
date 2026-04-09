#pragma once
#include "UID.h"

static const UID INVALID_REFERENCE_ID = -1;

constexpr const char* ASSETS_FOLDER = "Assets/";
constexpr const char* LIBRARY_FOLDER = "Library/";

constexpr const char* ASSET_EXTENSION = ".asset";
constexpr const char* METADATA_EXTENSION = ".metadata";

constexpr uint64_t NO_BINARY_ASSET = static_cast<uint64_t>(-1);