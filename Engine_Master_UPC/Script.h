#pragma once

#include "ScriptFieldInfo.h"

class GameObject;

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

    //Used when needing to rebuild runtime state after deserializing a field
    virtual void onAfterDeserialize() {}

    //Used when needing to rebuild runtime state after fixing references
    virtual void onAfterReferencesFixed() {}

    GameObject* getOwner() const { return m_owner; }

protected:
    GameObject* m_owner = nullptr;
};