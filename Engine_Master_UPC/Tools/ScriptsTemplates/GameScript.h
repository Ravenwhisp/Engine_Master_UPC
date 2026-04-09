#pragma once

#include "ScriptAPI.h"

class $safeitemname$ : public Script
{
    DECLARE_SCRIPT($safeitemname$)

public:
    explicit $safeitemname$(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;
};