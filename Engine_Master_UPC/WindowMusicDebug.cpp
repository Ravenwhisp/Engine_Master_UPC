#include "Globals.h"
#include "WindowMusicDebug.h"

#include "Application.h"
#include "ModuleMusic.h"
#include "WwiseBank.h"
#include "WwiseEvent.h"
#include "PlayingSound.h"

WindowMusicDebug::WindowMusicDebug()
{
	m_moduleMusic = app->getModuleMusic();
}

void WindowMusicDebug::drawInternal()
{
	static float playingSoundsHeight = 250.0f;

	if (!m_moduleMusic)
	{
		return;
	}

	const std::vector<WwiseBank>& banks = m_moduleMusic->getBankList();

	const float separatorThickness = 6.0f;
	const float minTopHeight = 100.0f;
	const float minBottomHeight = 120.0f;

	const float availableHeight = ImGui::GetContentRegionAvail().y;

	float topHeight = availableHeight - playingSoundsHeight - separatorThickness;

	if (topHeight < minTopHeight)
	{
		topHeight = minTopHeight;
		playingSoundsHeight = availableHeight - minTopHeight - separatorThickness;
	}

	if (playingSoundsHeight < minBottomHeight)
	{
		playingSoundsHeight = minBottomHeight;
		topHeight = availableHeight - minBottomHeight - separatorThickness;
	}

	if (ImGui::BeginChild("BanksRegion", ImVec2(0, topHeight), false))
	{
		for (const WwiseBank& bank : banks)
		{
			if (!ImGui::CollapsingHeader(bank.getName().c_str()))
			{
				continue;
			}

			const std::vector<WwiseEvent>& events = bank.getEvents();

			for (const WwiseEvent& event : events)
			{
				ImGui::Text("%s", event.name.c_str());

				ImGui::SameLine(250);

				const std::string buttonID = "Play##" + bank.getName() + event.name;

				if (ImGui::Button(buttonID.c_str()))
				{
					m_moduleMusic->postEvent(bank.getName().c_str(), event.name.c_str());
				}
			}
		}
	}

	ImGui::EndChild();

	ImGui::InvisibleButton("MusicSplitter", ImVec2(-1, separatorThickness));

	if (ImGui::IsItemActive())
	{
		playingSoundsHeight -= ImGui::GetIO().MouseDelta.y;
	}

	if (ImGui::IsItemHovered() || ImGui::IsItemActive())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
	}

	ImGui::Separator();

	if (ImGui::BeginChild("PlayingSoundsRegion", ImVec2(0, 0), true))
	{
		ImGui::Text("Playing Sounds");

		ImGui::Separator();

		const std::vector<PlayingSound>& playingSounds = m_moduleMusic->getPlayingSounds();

		if (playingSounds.empty())
		{
			ImGui::Text("No playing sounds");
		}
		else
		{
			for (const PlayingSound& playingSound : playingSounds)
			{
				const char* stateText = "";

				switch (playingSound.state)
				{
				case PlayingSoundState::Playing:
					stateText = "Playing";
					break;

				case PlayingSoundState::Paused:
					stateText = "Paused";
					break;

				case PlayingSoundState::Stopped:
					stateText = "Stopped";
					break;
				}

				ImGui::Text("Bank: %s", playingSound.bankName.c_str());
				ImGui::Text("Event: %s", playingSound.eventName.c_str());
				ImGui::Text("Playing ID: %u", playingSound.playingID);
				ImGui::Text("State: %s", stateText);

				if (playingSound.state == PlayingSoundState::Playing)
				{
					const std::string pauseButtonID = "Pause##" + std::to_string(playingSound.playingID);

					if (ImGui::Button(pauseButtonID.c_str()))
					{
						m_moduleMusic->pauseEvent(playingSound.playingID);
					}
				}
				else if (playingSound.state == PlayingSoundState::Paused)
				{
					const std::string resumeButtonID = "Resume##" + std::to_string(playingSound.playingID);

					if (ImGui::Button(resumeButtonID.c_str()))
					{
						m_moduleMusic->resumeEvent(playingSound.playingID);
					}
				}

				ImGui::SameLine();

				const std::string stopButtonID = "Stop##" + std::to_string(playingSound.playingID);

				if (ImGui::Button(stopButtonID.c_str()))
				{
					m_moduleMusic->stopEvent(playingSound.playingID);
				}

				ImGui::Separator();
			}
		}
	}

	ImGui::EndChild();
}