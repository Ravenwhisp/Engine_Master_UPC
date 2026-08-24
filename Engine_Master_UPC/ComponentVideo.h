#pragma once
#include "Component.h"

#include <string>

class ModuleVideo;
class VideoPlayback;

class ComponentVideo : public Component
{
private:
	ModuleVideo* m_moduleVideo = nullptr;
	VideoPlayback* m_playback = nullptr;

	std::string m_path;

public:
	ComponentVideo(UID id, GameObject* gameObject);
	~ComponentVideo();

	void play();
	void pause();
	void resume();
	void stop();

	bool isPlaying() const;
	bool isPaused() const;

	void setPath(const std::string& newPath) noexcept { m_path = newPath; }
	const std::string& getPath() const noexcept { return m_path; }

	void drawUi();

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;
	void serialize(IArchive& archive) override;
};