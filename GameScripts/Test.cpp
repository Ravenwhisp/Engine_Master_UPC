#include "pch.h"
#include "Test.h"
#include "ScriptAPI.h"

IMPLEMENT_SCRIPT(Test)

Test::Test(GameObject* owner)
    : Script(owner)
{
}

void Test::Start()
{
    OutputDebugStringA("Test::Start called\n");
}

void Test::Update()
{
    OutputDebugStringA("Test::Update running\n");
    if (getOwner())
    {
        OutputDebugStringA("Test::HasOwner\n");
        auto* transform = getOwner()->GetTransform();
        auto pos = transform->getPosition();
        pos.x += 0.005f;
        transform->setPosition(pos);
    }
}