#pragma once

class GameObject;

class Script
{
public:
    explicit Script(GameObject* owner)
        : m_owner(owner)
    {
    }

    virtual ~Script() = default;

    virtual void Start() {}
    virtual void Update() {}

    GameObject* getOwner() const { return m_owner; }

protected:
    GameObject* m_owner = nullptr;
};