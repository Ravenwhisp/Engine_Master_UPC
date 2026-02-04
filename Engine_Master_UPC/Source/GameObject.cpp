#include "Globals.h"
#include "GameObject.h"
#include "Transform.h"

#include "Application.h"
#include "GameCoreModule.h"

GameObject::GameObject()
{
    app->GetGameCoreModule()->CreateGameObject(this);
}

GameObject::GameObject(const std::string& name): _name(name)
{
    app->GetGameCoreModule()->CreateGameObject(this);

    AddComponent<Transform>();
}

bool GameObject::AddChild(GameObject* child)
{
    //Check if the child is already present
    if (!IsChild(child)) {
        children.push_back(child);
        child->SetParent(this);
        return true;
    }
    return false;
}

bool GameObject::RemoveChild(GameObject* child)
{
    if (IsChild(child)) {
        children.erase(std::remove(children.begin(), children.end(), child), children.end());
        child->SetParent(nullptr);
    }
    return false;
}

bool GameObject::IsChild(GameObject* potentialChild)
{
    for (auto child : children) {
        if (child == potentialChild) {
            return true;
        }
    }
    return false;
}

bool GameObject::IsChildOf(GameObject* parent)
{
    GameObject* p = GetParent();
    while (p)
    {
        if (p == parent)
            return true;
        p = p->GetParent();
    }
    return false;
}


