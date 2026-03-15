#pragma once

#include "Script.h"
#include <memory>

class Test : public Script
{
public:
    explicit Test(GameObject* owner);

    void Start() override;
    void Update() override;

    static std::unique_ptr<Script> Create(GameObject* owner);
};