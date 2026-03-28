#pragma once

#include "ScriptAPI.h"

class LocalMultiplayerSetup : public Script
{
    DECLARE_SCRIPT(LocalMultiplayerSetup)

public:
    explicit LocalMultiplayerSetup(GameObject* owner);

    void Start() override;

    ScriptFieldList getExposedFields() const override;

public:
    int m_setupMode = 0;
};