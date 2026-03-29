#pragma once

#include "ScriptAPI.h"

class LocalMultiplayerSetup : public Script
{
    DECLARE_SCRIPT(LocalMultiplayerSetup)

public:
    explicit LocalMultiplayerSetup(GameObject* owner);

    void Start() override;

    ScriptFieldList getExposedFields() const override;

    void setKeyboardGamepad();
    void setTwoGamepad();

private:
    void setMode(int mode);
    void chooseConfiguration();

public:
    int m_setupMode = 0;
};