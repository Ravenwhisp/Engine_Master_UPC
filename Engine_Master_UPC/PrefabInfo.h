#pragma once
#include "UID.h"
#include "ComponentType.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


struct PrefabOverrideRecord
{
    static constexpr const char* k_wholeComponentModified = "*";

    std::unordered_map<int, std::unordered_set<std::string>> m_modifiedProperties;
    std::vector<ComponentType> m_addedComponentTypes;
    std::vector<ComponentType> m_removedComponentTypes;

    bool isEmpty() const
    {
        return m_modifiedProperties.empty() && m_addedComponentTypes.empty() && m_removedComponentTypes.empty();
    }

    void clear()
    {
        m_modifiedProperties.clear();
        m_addedComponentTypes.clear();
        m_removedComponentTypes.clear();
    }
};

struct PrefabInfo
{
    UID                    m_assetUID;
    PrefabOverrideRecord   m_overrides;

    bool isInstance() const { return isValidUID(m_assetUID); }

    void clear()
    {
        m_assetUID = {};
        m_overrides.clear();
    }
};