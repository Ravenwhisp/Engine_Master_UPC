#include "Globals.h"
#include "AssetFileDialog.h"

#include "ModuleAssets.h"
#include "Asset.h"
#include "AssetDialogFilter.h"
#include "AssetsDictionary.h"
#include <FileDialog.h>
#include <thread>

void AssetFileDialog::requestSave(Asset& asset)
{
    if (m_running.load()) return;

    m_pendingAsset = &asset;
    m_pendingType = asset.getType();
    m_pendingIsSave = true;
    m_callback = nullptr;

    { std::lock_guard lock(m_resultMutex); m_result.reset(); }
    m_running.store(true);

    std::thread([this]()
        {
            const AssetDialogFilter filter = getDialogFilter(m_pendingType);
            auto result = saveAs(filter.filterSpec, filter.defaultExtension, "Save Asset", ASSETS_FOLDER);
            { std::lock_guard lock(m_resultMutex); m_result = std::move(result); }
            m_running.store(false);
        }).detach();
}

void AssetFileDialog::flush(ModuleAssets& assets)
{
    if (m_running.load()) return;

    std::optional<std::filesystem::path> result;
    {
        std::lock_guard lock(m_resultMutex);
        if (!m_result.has_value()) return;
        result = std::move(m_result);
        m_result.reset();
    }

    if (!result.has_value())
    {
        m_pendingAsset = nullptr;
        return;
    }

    if (m_pendingIsSave && m_pendingAsset)
    {
        assets.save(*m_pendingAsset, *result);
        m_pendingAsset = nullptr;
    }
    else if (!m_pendingIsSave && m_callback)
    {
        m_callback(*result);
        m_callback = nullptr;
    }
}

bool AssetFileDialog::isRunning() const
{
    return m_running.load();
}
