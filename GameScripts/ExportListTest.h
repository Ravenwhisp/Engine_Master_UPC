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
    int m_test1 = 0;
    float m_test2 = 0.0;
};