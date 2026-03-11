#include "pch.h"
#include "Test.h"

Test::Test(GameObject* owner)
    : Script(owner)
{
}

void Test::Start()
{
}

void Test::Update()
{
}

std::unique_ptr<Script> Test::Create(GameObject* owner)
{
    return std::make_unique<Test>(owner);
}