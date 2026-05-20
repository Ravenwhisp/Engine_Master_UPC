#pragma once

#include "EngineAPI.h"

class DataContainerAutoRegister
{
public:
    DataContainerAutoRegister(const char* name, const char* displayName, const char* extension, DataContainerCreator creator)
    {
        registerDataContainer(name, displayName, extension, creator);
    }
};

#define DECLARE_DATACONTAINER(TypeName) \
public: \
    static DataContainer* Create(AssetReference& uid);

#define IMPLEMENT_DATACONTAINER(TypeName, Extension) \
    DataContainer* TypeName::Create(AssetReference& uid) \
    { \
        return new TypeName(uid); \
    } \
    static DataContainerAutoRegister s_autoRegister_##TypeName( \
        #TypeName, \
        #TypeName, \
        Extension, \
        &TypeName::Create \
    );
