#pragma once
#include "AssetType.h"
#include <atomic>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>

class Asset;
class ModuleAssets;

class AssetFileDialog
{
public:
    void requestSave(Asset& asset);
    void flush(ModuleAssets& assets);
    bool isRunning() const;

private:
    std::atomic<bool>                                    m_running{ false };
    std::mutex                                           m_resultMutex;
    std::optional<std::filesystem::path>                 m_result;
    std::function<void(const std::filesystem::path&)>    m_callback;
    Asset*                                               m_pendingAsset = nullptr;
    AssetType                                            m_pendingType = AssetType::UNKNOWN;
    bool                                                 m_pendingIsSave = false;
};
