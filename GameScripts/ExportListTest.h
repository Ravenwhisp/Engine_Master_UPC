#pragma once

#include "ScriptAPI.h"

class ExportListTest : public Script
{
    DECLARE_SCRIPT(ExportListTest)

public:
    explicit ExportListTest(GameObject* owner);

    void Start() override;
    void Update() override;

    FieldList getExposedFields() const override;

private:
    ComponentRefList m_transforms;
    int m_test1 = 0;
    float m_test2 = 0.0;
};