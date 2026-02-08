#pragma once
#include "ComponentType.h"

class GameObject;

class Component {
public:
    Component(int id, ComponentType type, GameObject* gameObject) : m_uuid(id), m_type(type), m_owner(gameObject) {}
    virtual ~Component() = default;

    int getID() const { return m_uuid; }
    ComponentType getType() const { return m_type; }
    GameObject* getOwner() const { return m_owner; }

    virtual void drawUi() {}

protected:
    GameObject* m_owner;

private:
    const int m_uuid;
    const ComponentType m_type;
};
