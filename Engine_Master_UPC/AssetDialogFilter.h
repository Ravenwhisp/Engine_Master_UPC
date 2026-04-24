#pragma once

#include "AssetType.h"

struct AssetDialogFilter
{
    const char* filterSpec;
    const char* defaultExtension;
};

AssetDialogFilter getDialogFilter(AssetType type);