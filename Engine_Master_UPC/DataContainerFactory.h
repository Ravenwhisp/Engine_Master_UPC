#pragma once

#include "DataContainerRegistry.h"
#include <vector>

struct AssetReference;

class DataContainerFactory
{
public:
    static void registerDataContainer(const std::string& name, const std::string& displayName, const std::string& extension, DataContainerCreator creator);
    static DataContainer* createDataContainer(const std::string& name, AssetReference& uid);
    static bool isDataContainerRegistered(const std::string& name);
    static const std::vector<DataContainerRegistry>& getAllRegistered();

private:
    static std::vector<DataContainerRegistry> m_registry;
};
