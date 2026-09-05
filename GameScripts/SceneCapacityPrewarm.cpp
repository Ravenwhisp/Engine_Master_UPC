#include "pch.h"
#include "SceneCapacityPrewarm.h"

IMPLEMENT_SCRIPT_FIELDS(SceneCapacityPrewarm,
	SERIALIZED_INT(
		m_objectCount,
		"Object Count",
		0,
		1000,
		10
	)
)

SceneCapacityPrewarm::SceneCapacityPrewarm(GameObject* owner)
	: Script(owner)
{
}

void SceneCapacityPrewarm::Update()
{
	switch (m_state)
	{
	case State::Create:
	{
		Debug::log(
			"[SceneCapacityPrewarm] Creating %d temporary GameObjects.",
			m_objectCount
		);

		m_dummyObjects.reserve(m_objectCount);

		for (int i = 0; i < m_objectCount; ++i)
		{
			GameObject* dummy =
				GameObjectAPI::createGameObject(
					"CapacityPrewarm"
				);

			if (!dummy)
			{
				continue;
			}

			// No reason for these objects to update/render/do anything.
			GameObjectAPI::setActive(
				dummy,
				false
			);

			m_dummyObjects.push_back(
				dummy
			);
		}

		m_state = State::Remove;

		break;
	}

	case State::Remove:
	{
		Debug::log(
			"[SceneCapacityPrewarm] Removing %d temporary GameObjects.",
			static_cast<int>(m_dummyObjects.size())
		);

		for (GameObject* dummy : m_dummyObjects)
		{
			if (!dummy)
			{
				continue;
			}

			GameObjectAPI::removeGameObject(
				dummy
			);
		}

		m_dummyObjects.clear();

		m_state = State::Done;

		break;
	}

	case State::Done:
	default:
		break;
	}
}

IMPLEMENT_SCRIPT(SceneCapacityPrewarm)