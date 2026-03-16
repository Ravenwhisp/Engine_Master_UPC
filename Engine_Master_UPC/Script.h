#pragma once

#include "ScriptFieldInfo.h"

class GameObject;

struct ScriptFieldList
{
    const ScriptFieldInfo* fields = nullptr;
    size_t count = 0;
};

class Script
{
public:
    explicit Script(GameObject* owner) : m_owner(owner) {}
    virtual ~Script() = default;

    virtual void Start() {}
    virtual void Update() {}

    virtual ScriptFieldList getExposedFields() const
    {
        return {};
    }

    //Used when changing a field in the inspector must trigger some extra action
    virtual void onFieldEdited(const ScriptFieldInfo& field) {}

    //Used when needing to rebuild derived/runtime state after deserializing a field
    virtual void onAfterDeserialize() {}

    GameObject* getOwner() const { return m_owner; }

protected:
    GameObject* m_owner = nullptr;
};