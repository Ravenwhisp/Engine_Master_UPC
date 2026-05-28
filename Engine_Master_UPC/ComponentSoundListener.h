#pragma once
#include "Component.h"

class ModuleMusic;

class ComponentSoundListener : public Component
{
private:
	ModuleMusic* m_moduleMusic = nullptr;
	uint64_t m_audioGameObjectID = 0;

public:
	ComponentSoundListener(UID id, GameObject* gameObject);
	~ComponentSoundListener();

#pragma region GameLoop
	bool init() override;
	void update() override;
	bool cleanUp() override;
#pragma endregion

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void serialize(IArchive& archive) override;
};