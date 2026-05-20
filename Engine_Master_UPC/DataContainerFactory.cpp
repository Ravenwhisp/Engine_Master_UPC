#include "Globals.h"
#include "DataContainerFactory.h"
#include "AssetReference.h"

std::vector<DataContainerRegistry> DataContainerFactory::m_registry;

void DataContainerFactory::registerDataContainer(const std::string& name, const std::string& displayName, DataContainerCreator creator)
{
    for (DataContainerRegistry& entry : m_registry)
    {
        if (entry.name == name)
        {
            entry.creator = creator;
            return;
        }
    }

    m_registry.push_back({ name, displayName, creator });
}

DataContainer* DataContainerFactory::createDataContainer(const std::string& name, AssetReference& uid)
{
    for (const DataContainerRegistry& entry : m_registry)
    {
        if (entry.name == name)
        {
            return entry.creator(uid);
        }
    }

    return nullptr;
}

bool DataContainerFactory::isDataContainerRegistered(const std::string& name)
{
    for (const DataContainerRegistry& entry : m_registry)
    {
        if (entry.name == name)
        {
            return true;
        }
    }

    return false;
}

const std::vector<DataContainerRegistry>& DataContainerFactory::getAllRegistered()
{
    return m_registry;
}