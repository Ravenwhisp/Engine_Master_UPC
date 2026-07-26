#pragma once
#include "EditorWindow.h"
#include "DataContainer.h"

#include <filesystem>
#include <memory>
#include <vector>

class WindowDataContainerManager : public EditorWindow
{
public:
    WindowDataContainerManager();
    const char* getWindowName() const override { return "Data Container Manager"; }
    void drawInternal() override;

private:
    struct Entry
    {
        std::filesystem::path path;
        std::string typeName;
        std::string displayName;
        std::shared_ptr<DataContainer> asset;
    };

    void scanFiles();
    void ensureLoaded(int i);
    void drawList();
    void drawDetail();

    std::vector<Entry> m_entries;
    int m_selected = -1;
    bool m_needsScan = true;
    char m_filter[128] = "";
};
