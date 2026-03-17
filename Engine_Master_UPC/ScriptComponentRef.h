#pragma once

#include "UID.h"
#include "Component.h"

template<typename T>
struct ScriptComponentRef
{
    UID uid = 0;
    Component* component = nullptr;

    T* get() const
    {
        return static_cast<T*>(component);
    }

    void clear()
    {
        uid = 0;
        component = nullptr;
    }

    bool isValid() const
    {
        return uid != 0 && component != nullptr;
    }
};

struct ScriptComponentRefStorage
{
    UID uid;
    Component* component;
};