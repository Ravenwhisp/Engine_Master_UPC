#include "Globals.h"
#include "Component.h"

#include "GameObject.h"

Transform* Component::getTransform()
{
    return m_owner->GetTransform();
}
