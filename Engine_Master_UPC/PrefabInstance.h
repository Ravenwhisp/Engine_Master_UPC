#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>

#include "MD5Fwd.h"

struct PrefabOverrideRecord
{
    std::unordered_map<int, std::unordered_set<std::string>> m_modifiedProperties;
    std::vector<int> m_addedComponentTypes;
    std::vector<int> m_removedComponentTypes;

    bool isEmpty() const
    {
        return m_modifiedProperties.empty()
            && m_addedComponentTypes.empty()
            && m_removedComponentTypes.empty();
    }

    void clear()
    {
        m_modifiedProperties.clear();
        m_addedComponentTypes.clear();
        m_removedComponentTypes.clear();
    }
};

struct PrefabInstanceInfo
{
    std::filesystem::path m_sourcePath;
    MD5Hash m_assetUID;

    PrefabOverrideRecord m_overrides;

    bool isInstance() const
    {
        return !m_sourcePath.empty();
    }

    std::string getName() const
    {
        return m_sourcePath.stem().string();
    }

    void clear()
    {
        m_sourcePath.clear();
        m_assetUID = {};
        m_overrides.clear();
    }
};