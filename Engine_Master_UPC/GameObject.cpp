 #include "GameObject.h"

GameObject::GameObject(short newUuid) : m_uuid(newUuid)
{

}

GameObject::~GameObject()
{
}

bool GameObject::AddComponent(Component* newComponent)
{
    if (newComponent->getType() == TRANSFORM) return false;

    m_components.push_back(newComponent);
    return true;
}

bool GameObject::RemoveComponent(Component* componentToRemove)
{
    m_components.erase(std::remove_if(m_components.begin(), m_components.end(), [componentToRemove](Component* c)
        {
            if (c == componentToRemove)
            {
                delete c;
                return true;
            }
            return false;
        }), m_components.end());
}