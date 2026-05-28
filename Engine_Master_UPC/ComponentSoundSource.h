#pragma once

#include "Component.h"

#include <cstdint>

class ModuleMusic;

class ComponentSoundSource : public Component
{
private:
	ModuleMusic* m_moduleMusic = nullptr;
	uint64_t m_audioGameObjectID = 0;

public:
	ComponentSoundSource(UID id, GameObject* gameObject);
	~ComponentSoundSource();

#pragma region GameLoop
	bool init() override;
	void update() override;
	bool cleanUp() override;
#pragma endregion

	uint32_t postEvent(const char* bankName, const char* eventName);
	void stopEvent(uint32_t playingID);
	void pauseEvent(uint32_t playingID);
	void resumeEvent(uint32_t playingID);

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;
	void drawUi() override;

	void serialize(IArchive& archive) override;
};