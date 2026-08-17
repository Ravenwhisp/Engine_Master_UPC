#include "Globals.h"
#include "WindowVideoDebug.h"

#include "Application.h"
#include "ModuleVideo.h"
#include "VideoPlayback.h"

#include <filesystem>
#include <string>

WindowVideoDebug::WindowVideoDebug()
{
	m_moduleVideo = app->getModuleVideo();
}

void WindowVideoDebug::drawInternal()
{
	if (!m_moduleVideo)
		return;

	ImGui::Text("Video Tester");
	ImGui::Separator();

	static char videoPath[512] = "";

	ImGui::InputText("Video Path", videoPath, IM_ARRAYSIZE(videoPath));

	if (ImGui::Button("Play Video", ImVec2(-1.0f, 0.0f)))
	{
		if (videoPath[0] != '\0')
			m_moduleVideo->playVideo(videoPath);
	}

	ImGui::Spacing();
	ImGui::Separator();

	const auto& activeVideos = m_moduleVideo->getActiveVideos();

	ImGui::Text("Active Videos");
	ImGui::SameLine();
	ImGui::TextDisabled("(%zu)", activeVideos.size());

	ImGui::Separator();

	if (activeVideos.empty())
	{
		ImGui::Spacing();
		ImGui::TextDisabled("No active videos");
		return;
	}

	VideoPlayback* videoToStop = nullptr;

	if (ImGui::BeginTable("ActiveVideosTable", 6, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
	{
		ImGui::TableSetupColumn("Video");
		ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 120.0f);
		ImGui::TableSetupColumn("Video Stream", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableSetupColumn("Audio Stream", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 140.0f);

		ImGui::TableHeadersRow();

		for (const auto& videoPtr : activeVideos)
		{
			VideoPlayback* video = videoPtr.get();

			if (!video)
				continue;

			ImGui::PushID(video);
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);

			const std::filesystem::path& path = video->getPath();
			const std::string filename = path.filename().string();

			ImGui::TextUnformatted(filename.c_str());

			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", path.string().c_str());
			}

			ImGui::TableSetColumnIndex(1);

			const char* state = "Stopped";

			if (video->isPlaying())
			{
				state = "Playing";
			}
			else if (video->isPaused())
			{
				state = "Paused";
			}
			else if (video->isFinished())
			{
				state = "Finished";
			}

			ImGui::TextUnformatted(state);

			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%.2f / %.2f", video->getCurrentVideoTime(), video->getDuration());

			ImGui::TableSetColumnIndex(3);

			if (video->getVideoStreamIndex() >= 0)
			{
				ImGui::Text("%d", video->getVideoStreamIndex());
			}
			else
			{
				ImGui::TextDisabled("-");
			}

			ImGui::TableSetColumnIndex(4);

			if (video->getAudioStreamIndex() >= 0)
			{
				ImGui::Text("%d", video->getAudioStreamIndex());
			}
			else
			{
				ImGui::TextDisabled("-");
			}

			ImGui::TableSetColumnIndex(5);

			if (video->isPlaying())
			{
				if (ImGui::SmallButton("Pause"))
				{
					video->pause();
				}
			}
			else if (video->isPaused())
			{
				if (ImGui::SmallButton("Resume"))
				{
					video->play();
				}
			}
			else
			{
				if (ImGui::SmallButton("Play"))
				{
					video->play();
				}
			}

			ImGui::SameLine();

			if (ImGui::SmallButton("Stop"))
			{
				videoToStop = video;
			}

			ImGui::PopID();
		}

		ImGui::EndTable();
	}

	if (videoToStop)
	{
		m_moduleVideo->stopVideo(videoToStop);
	}

	ImGui::Spacing();
	ImGui::Separator();

	if (ImGui::Button("Stop All Videos", ImVec2(-1.0f, 0.0f)))
	{
		m_moduleVideo->stopAllVideos();
	}
}