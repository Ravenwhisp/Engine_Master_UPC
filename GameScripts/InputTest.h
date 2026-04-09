#pragma once

#include "ScriptAPI.h"

#include <vector>

class InputTest : public Script
{
    DECLARE_SCRIPT(InputTest)

public:
    explicit InputTest(GameObject* owner);

    void Start() override;
    void Update() override;

private:

};