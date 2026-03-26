#pragma once

#include "UID.h"
#include "Component.h"

template<typename T>
struct ScriptComponentRef
{
    UID uid = 0;
    Component* component = nullptr;

    T* getReferencedComponent() const
    {
        return static_cast<T*>(component);
    }
};