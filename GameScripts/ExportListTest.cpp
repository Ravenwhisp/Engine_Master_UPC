#include "pch.h"
#include "ExportListTest.h"

IMPLEMENT_SCRIPT_FIELDS(ExportListTest,
    SERIALIZED_COMPONENT_REF_LIST(m_transforms, "TransformsList", ComponentType::TRANSFORM)
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