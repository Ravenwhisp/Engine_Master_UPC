#include "pch.h"
#include "TriggerDebugScript.h"

TriggerDebugScript::TriggerDebugScript(GameObject* owner)
    : Script(owner)
{
}

void TriggerDebugScript::OnTriggerEnter(GameObject* other)
{
    if (!other)
    {
        return;
    }

    Debug::log("[TriggerDebugScript] %s entered trigger with %s", getOwner()->GetName().c_str(), other->GetName().c_str());
}

void TriggerDebugScript::OnTriggerExit(GameObject* other)
{
    if (!other)
    {
        return;
    }

    Debug::log("[TriggerDebugScript] %s exited trigger with %s", getOwner()->GetName().c_str(), other->GetName().c_str());
}

IMPLEMENT_SCRIPT(TriggerDebugScript)