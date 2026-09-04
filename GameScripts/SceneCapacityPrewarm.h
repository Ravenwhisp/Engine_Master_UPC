#pragma once

#include "ScriptAPI.h"

#include <vector>

class SceneCapacityPrewarm : public Script
{
	DECLARE_SCRIPT(Spawn)

public:
	explicit SceneCapacityPrewarm(GameObject* owner);

	FieldList getExposedFields() const override;

	void Update() override;

public:
	int m_objectCount = 200;

private:
	enum class State
	{
		Create,
		Remove,
		Done
	};

	State m_state = State::Create;

	std::vector<GameObject*> m_dummyObjects;
};