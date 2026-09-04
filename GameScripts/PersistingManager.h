#pragma once

#include "ScriptAPI.h"

class PersistingManager : public Script
{
    DECLARE_SCRIPT(PersistingManager)

public:
    explicit PersistingManager(GameObject* owner);

    void Start() override;
    void OnGameStop() override;

    FieldList getExposedFields() const override;

public:
    int m_levelNumber = 0;
};