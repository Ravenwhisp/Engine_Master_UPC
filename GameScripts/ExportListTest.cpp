#include "pch.h"
#include "ExportListTest.h"

IMPLEMENT_SCRIPT_FIELDS(ExportListTest,
    SERIALIZED_INT(m_test1, "Test One"),
    SERIALIZED_COMPONENT_REF_LIST(m_transforms, "TransformsList", ComponentType::TRANSFORM),
    SERIALIZED_FLOAT(m_test2, "Test Two", 0.0, 50.0, 1.0)
)

ExportListTest::ExportListTest(GameObject* owner)
    : Script(owner)
{
}

void ExportListTest::Start()
{
    Debug::log("[ExportListTest] Start - entries: %d", static_cast<int>(m_transforms.size()));
}

void ExportListTest::Update()
{
}

IMPLEMENT_SCRIPT(ExportListTest)