#include "pch.h"
#include "TestingButton.h"

static void ButtonHover_Func(Script* s)
{
	static_cast<TestingButton*>(s)->ButtonHover();
}
static void ButtonPress_Func(Script* s)
{
	static_cast<TestingButton*>(s)->ButtonPress();
}
static void ButtonRelease_Func(Script* s)
{
	static_cast<TestingButton*>(s)->ButtonRelease();
}

static const ScriptMethodInfo TestingButtonMethods[] =
{
	{ "ButtonHover", &ButtonHover_Func },
	{ "ButtonPress", &ButtonPress_Func },
	{ "ButtonRelease", &ButtonRelease_Func }
}; 
ScriptMethodList TestingButton::getExposedMethods() const
{
	return { TestingButtonMethods, sizeof(TestingButtonMethods) / sizeof(ScriptMethodInfo) };
}

TestingButton::TestingButton(GameObject* owner) : Script(owner) {}

void TestingButton::Start()
{

}

void TestingButton::Update()
{

}

void TestingButton::onFieldEdited(const ScriptFieldInfo& field)
{

}

void TestingButton::onAfterDeserialize()
{

}

void TestingButton::ButtonHover()
{
	Debug::log("Button hovered!");
}

void TestingButton::ButtonPress()
{
	Debug::log("Button pressed!");
}

void TestingButton::ButtonRelease()
{
	Debug::log("Button released!");
}

IMPLEMENT_SCRIPT(TestingButton)
