#include "pch.h"
#include "NavBlockerTest.h"

NavBlockerTest::NavBlockerTest(GameObject* owner)
	: Script(owner)
{
}

void NavBlockerTest::Start()
{
	m_blocker = NavigationAPI::getRuntimeBlockerComponent(getOwner());

	if (!m_blocker)
	{
		Debug::warn("[NavBlockerTest] NavRuntimeBlockerComponent not found on owner.");
	}
}

void NavBlockerTest::Update()
{
	if (!m_blocker)
	{
		return;
	}

	if (Input::isFaceButtonBottomJustPressed()) // Space button click
	{
		const bool newState = !NavigationAPI::isBlocked(m_blocker);

		NavigationAPI::setBlocked(m_blocker, newState);

		Debug::log(
		"[NavBlockerTest] Runtime blocker is now %s", newState ? "BLOCKED" : "UNBLOCKED");
	}
}

IMPLEMENT_SCRIPT(NavBlockerTest)