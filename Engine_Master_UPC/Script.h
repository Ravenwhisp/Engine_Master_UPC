#pragma once

#include "IFieldContainer.h"

#include "ScriptMethodInfo.h"
#include "ScriptMethodList.h"

class GameObject;

class Script : public IFieldContainer
{
public:
    explicit Script(GameObject* owner) : m_owner(owner) {}
    virtual ~Script() = default;

    virtual void Start() {}
    virtual void Update() {}

    virtual void OnGameStop() {}

    virtual void OnTriggerEnter(GameObject* gameObject) {}
    virtual void OnTriggerExit(GameObject* gameObject) {}

    virtual ScriptMethodList getExposedMethods() const
    {
        return {};
    }

    //Used when needing to rebuild runtime state after fixing references
    virtual void onAfterReferencesFixed() {}

    GameObject* getOwner() const { return m_owner; }

    virtual void drawGizmo() {}

protected:
    GameObject* m_owner = nullptr;
};
