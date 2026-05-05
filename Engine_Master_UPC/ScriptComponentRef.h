#pragma once

#include "UID.h"
#include "Component.h"

#include <vector>

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

using ScriptComponentRefList = std::vector<ScriptComponentRef<Component>>;