#pragma once

#include "ScriptAPI.h"

class NavBlockerTest : public Script
{
	DECLARE_SCRIPT(NavBlockerTest)

public:
	explicit NavBlockerTest(GameObject* owner);

	void Start() override;
	void Update() override;

private:
	NavRuntimeBlockerComponent* m_blocker = nullptr;
};