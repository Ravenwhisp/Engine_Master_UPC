#pragma once
#include "Component.h"
#include "AssetReference.h"
#include "VideoAsset.h"

class ModuleVideo;
class VideoPlayback;

class ComponentVideo : public Component
{
private:
	ModuleVideo* m_moduleVideo = nullptr;
	VideoPlayback* m_playback = nullptr;

	VideoRef m_asset;

public:
	ComponentVideo(UID id, GameObject* gameObject);
	~ComponentVideo();

	void play();
	void pause();
	void resume();
	void stop();

	bool isPlaying() const;
	bool isPaused() const;

	void setAsset(const VideoRef& ref) noexcept { m_asset = ref; }
	const VideoRef& getAsset() const noexcept { return m_asset; }

	void drawUi();

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;
	void serialize(IArchive& archive) override;
};