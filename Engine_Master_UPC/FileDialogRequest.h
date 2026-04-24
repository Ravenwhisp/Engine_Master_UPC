#pragma once
#include "AssetType.h"
#include <filesystem>
#include <functional>

struct FileDialogRequest
{
    enum class Type { Save, Open };

    Type       type = Type::Save;
    AssetType  assetType = AssetType::UNKNOWN;

    std::function<void(const std::filesystem::path&)> onConfirm;
};