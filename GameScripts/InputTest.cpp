#include "pch.h"
#include "InputTest.h"

InputTest::InputTest(GameObject* owner)
    : Script(owner)
{
}

void InputTest::Start()
{
}

void InputTest::Update()
{
    if(Input::isFaceButtonBottomPressed(1))
    {
        Debug::log("FaceButtonBottom Pressed");
    }

    if (Input::isFaceButtonBottomJustPressed(1))
    {
        Debug::log("FaceButtonBottom JustPressed");
    }

    if (Input::isFaceButtonBottomReleased(1))
    {
        Debug::log("FaceButtonBottom Released");
    }
}

IMPLEMENT_SCRIPT(InputTest)