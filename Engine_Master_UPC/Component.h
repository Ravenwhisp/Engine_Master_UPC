#pragma once

class GameObject;
enum class ComponentType
{
    TRANSFORM = 0,
    COUNT
};

class Component {
public:
    virtual ~Component() = default;

    const short getID() { return m_uuid; }
    const ComponentType getType() { return m_type; }

    virtual void drawUi() {

    }

protected:
    ComponentType m_type;
    GameObject* m_gameObject;

private:
    short m_uuid;
};
