#pragma once

class GameObject;
enum ComponentType
{
    TRANSFORM
};

class Component {
public:
    virtual ~Component() = default;

    const short getID() { return m_uuid; }
    const ComponentType getType() { return m_type; }

    virtual void drawUi();

protected:
    ComponentType m_type;

private:
    short m_uuid;
};
