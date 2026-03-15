#pragma once

#include "Script.h"
#include "ScriptAutoRegister.h"
#include <memory>

class Test : public Script
{
    DECLARE_SCRIPT(Test)

public:
    explicit Test(GameObject* owner);

    void Start() override;
    void Update() override;
};