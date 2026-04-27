#pragma once

#include "ScriptAPI.h"

class TriggerDebugScript : public Script
{
    DECLARE_SCRIPT(TriggerDebugScript)
public:
    explicit TriggerDebugScript(GameObject* owner);

    void OnTriggerEnter(GameObject* other) override;
    void OnTriggerExit(GameObject* other) override;
};