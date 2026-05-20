#pragma once

#include "EngineAPI.h"

class DataContainerAutoRegister
{
public:
    DataContainerAutoRegister(const char* name, const char* displayName, DataContainerCreator creator)
    {
        registerDataContainer(name, displayName, creator);
    }
};

#define DECLARE_DATACONTAINER(TypeName) \
public: \
    static DataContainer* Create(AssetReference& uid);

#define IMPLEMENT_DATACONTAINER(TypeName) \
    DataContainer* TypeName::Create(AssetReference& uid) \
    { \
        return new TypeName(uid); \
    } \
    static DataContainerAutoRegister s_autoRegister_##TypeName( \
        #TypeName, \
        #TypeName, \
        &TypeName::Create \
    );