#include "Globals.h"
#include "ComponentVideo.h"

#include "Application.h"
#include "ModuleVideo.h"
#include "ModuleAssets.h"
#include "VideoAsset.h"
#include "VideoPlayback.h"
#include "IArchive.h"

#include "imgui.h"

#include <cstring>

ComponentVideo::ComponentVideo(UID id, GameObject* gameObject) : Component(id, ComponentType::VIDEO, gameObject)
{
	m_moduleVideo = app->getModuleVideo();
}

ComponentVideo::~ComponentVideo() = default;

void ComponentVideo::play()
{
	if (!m_moduleVideo || !m_asset.m_id.isValid())
		return;

	if (m_playback)
	{
		m_playback->play();
		return;
	}

	const VideoAsset* asset = m_asset.get();

	if (!asset)
	{
		auto loaded = app->getModuleAssets()->load<VideoAsset>(m_asset.m_id);
		if (loaded)
		{
			m_asset.m_data = loaded;
			asset = loaded.get();
		}
	}

	if (!asset || asset->getVideoData().empty())
		return;

	m_playback = m_moduleVideo->playVideo(asset->getVideoData());
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
	ImGui::Text("Video Asset:");
	ImGui::SameLine();

	if (m_asset.m_id.isValid())
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Assigned (UID %llu)", m_asset.m_id.m_uid);
	else
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "None");

	ImGui::Button("Drop Video Asset Here");
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
		{
			const UID droppedUID = *static_cast<const UID*>(payload->Data);
			AssetId* resolved = app->getModuleAssets()->findReference(droppedUID);
			if (resolved)
			{
				if (resolved->m_type == AssetType::VIDEO)
				{
					m_asset.m_id = *resolved;
					auto loaded = app->getModuleAssets()->load<VideoAsset>(*resolved);
					if (loaded)
						m_asset.m_data = loaded;
				}
				delete resolved;
			}
		}
		ImGui::EndDragDropTarget();
	}

	if (m_asset.m_id.isValid())
	{
		ImGui::SameLine();
		if (ImGui::Button("Clear"))
			m_asset = VideoRef();
	}

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
	cloned->setAsset(m_asset);

	return cloned;
}

void ComponentVideo::serialize(IArchive& archive)
{
	Component::serialize(archive);
	archive.beginObject("asset");
	m_asset.serialize(archive);
	archive.endObject();
}