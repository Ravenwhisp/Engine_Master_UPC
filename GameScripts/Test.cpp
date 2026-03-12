#include "pch.h"
#include "Test.h"
#include "GameObject.h"

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

std::unique_ptr<Script> Test::Create(GameObject* owner)
{
    return std::make_unique<Test>(owner);
}