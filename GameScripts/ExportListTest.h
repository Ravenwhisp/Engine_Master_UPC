#pragma once

#include "ScriptAPI.h"

class ExportListTest : public Script
{
    DECLARE_SCRIPT(ExportListTest)

public:
    explicit ExportListTest(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

private:
    ScriptComponentRefList m_transforms;
};