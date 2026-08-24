#include "Globals.h"
#include "ComponentVideo.h"

#include "Application.h"
#include "ModuleVideo.h"
#include "VideoPlayback.h"
#include "IArchive.h"

#include <cstring>

ComponentVideo::ComponentVideo(UID id, GameObject* gameObject) : Component(id, ComponentType::VIDEO, gameObject)
{
	m_moduleVideo = app->getModuleVideo();
}

ComponentVideo::~ComponentVideo() = default;

void ComponentVideo::play()
{
	if (!m_moduleVideo || m_path.empty())
		return;

	if (m_playback)
	{
		m_playback->play();
		return;
	}

	m_playback = m_moduleVideo->playVideo(m_path);
}

void ComponentVideo::pause()
{
	if (!m_playback)
		return;

	m_playback->pause();
}

void ComponentVideo::resume()
{
	if (!m_playback)
		return;

	m_playback->play();
}

void ComponentVideo::stop()
{
	if (!m_moduleVideo || !m_playback)
		return;

	m_moduleVideo->stopVideo(m_playback);
	m_playback = nullptr;
}

bool ComponentVideo::isPlaying() const
{
	return m_playback && m_playback->isPlaying();
}

bool ComponentVideo::isPaused() const
{
	return m_playback && m_playback->isPaused();
}

void ComponentVideo::drawUi()
{
	char pathBuffer[512] = {};
	strcpy_s(pathBuffer, m_path.c_str());

	if (ImGui::InputText("Video Path", pathBuffer, sizeof(pathBuffer)))
		m_path = pathBuffer;

	if (!m_playback)
	{
		if (ImGui::Button("Play"))
			play();

		return;
	}

	if (m_playback->isPlaying())
	{
		if (ImGui::Button("Pause"))
			pause();
	}
	else if (m_playback->isPaused())
	{
		if (ImGui::Button("Resume"))
			resume();
	}
	else
	{
		if (ImGui::Button("Play"))
			play();
	}

	ImGui::SameLine();

	if (ImGui::Button("Stop"))
		stop();
}

std::unique_ptr<Component> ComponentVideo::clone(GameObject* newOwner) const
{
	auto cloned = std::make_unique<ComponentVideo>(m_uuid, newOwner);

	cloned->setActive(isActive());
	cloned->setPath(m_path);

	return cloned;
}

void ComponentVideo::serialize(IArchive& archive)
{
	Component::serialize(archive);
	archive.serialize(m_path, "path");
}