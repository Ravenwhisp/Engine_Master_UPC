#pragma once
#include "ComponentType.h"
#include "UID.h" 

#include <rapidjson/document.h>

class GameObject;

class Component {
public:
    Component(UID id, ComponentType type, GameObject* gameObject) : m_uuid(id), m_type(type), m_owner(gameObject) {}
    virtual ~Component() = default;

    UID getID() const { return m_uuid; }
    ComponentType getType() const { return m_type; }
    GameObject* getOwner() const { return m_owner; }

    #pragma region Loop functions
    virtual bool init() { return true; }
    virtual bool postInit() { return true; }
    virtual void update() {}
    virtual void preRender() {}
    virtual void postRender() {}
    virtual void render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix) {}
    virtual bool cleanUp() { return true; }
    #pragma endregion

    virtual void drawUi() {}

    virtual void onTransformChange() {};

    virtual rapidjson::Value getJSON(rapidjson::Document& domTree) { return rapidjson::Value(); }; // for serialization

protected:
    GameObject* m_owner;

    const UID m_uuid;

private:
    const ComponentType m_type;
};
