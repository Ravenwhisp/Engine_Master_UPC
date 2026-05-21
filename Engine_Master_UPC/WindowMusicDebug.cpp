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
	static float playingSoundsHeight = 260.0f;

	if (!m_moduleMusic)
	{
		return;
	}

	const std::vector<WwiseBank>& banks = m_moduleMusic->getBankList();

	const float separatorThickness = 6.0f;
	const float minTopHeight = 120.0f;
	const float minBottomHeight = 140.0f;
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

	ImGui::Text("Audio Banks");
	ImGui::Separator();

	if (ImGui::BeginChild("BanksRegion", ImVec2(0, topHeight), true))
	{
		for (const WwiseBank& bank : banks)
		{
			const std::vector<WwiseEvent>& events = bank.getEvents();

			std::string headerLabel =
				bank.getName() + " (" + std::to_string(events.size()) + " events)";

			if (!bank.isLoaded())
			{
				headerLabel += " [UNLOADED]";
			}

			if (!ImGui::CollapsingHeader(headerLabel.c_str()))
			{
				continue;
			}

			if (!bank.isLoaded())
			{
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 180, 80, 255));

				ImGui::TextWrapped(
					"This bank is currently unloaded.\n"
					"Load it from Scene Config to play events."
				);

				ImGui::PopStyleColor();

				continue;
			}

			for (const WwiseEvent& event : events)
			{
				ImGui::PushID((bank.getName() + event.name).c_str());

				ImGui::TextUnformatted(event.name.c_str());

				ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f);

				if (ImGui::Button("Play", ImVec2(60.0f, 0.0f)))
				{
					m_moduleMusic->postGLobalEvent(bank.getName().c_str(), event.name.c_str());
				}

				ImGui::PopID();
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
		const std::vector<PlayingSound>& playingSounds = m_moduleMusic->getPlayingSounds();

		ImGui::Text("Playing Sounds");
		ImGui::SameLine();
		ImGui::TextDisabled("(%zu)", playingSounds.size());

		ImGui::Separator();

		if (playingSounds.empty())
		{
			ImGui::Spacing();
			ImGui::TextDisabled("No playing sounds");
		}
		else
		{
			if (ImGui::BeginTable("PlayingSoundsTable", 5, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
			{
				ImGui::TableSetupColumn("Bank");
				ImGui::TableSetupColumn("Event");
				ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 80.0f);
				ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 90.0f);
				ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 170.0f);
				ImGui::TableHeadersRow();

				for (const PlayingSound& playingSound : playingSounds)
				{
					const char* stateText = "Unknown";

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

					ImGui::PushID(static_cast<int>(playingSound.playingID));

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(playingSound.bankName.c_str());

					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted(playingSound.eventName.c_str());

					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%u", playingSound.playingID);

					ImGui::TableSetColumnIndex(3);
					ImGui::TextUnformatted(stateText);

					ImGui::TableSetColumnIndex(4);

					if (playingSound.state == PlayingSoundState::Playing)
					{
						if (ImGui::SmallButton("Pause"))
						{
							m_moduleMusic->pauseEvent(playingSound.playingID);
						}
					}
					else if (playingSound.state == PlayingSoundState::Paused)
					{
						if (ImGui::SmallButton("Resume"))
						{
							m_moduleMusic->resumeEvent(playingSound.playingID);
						}
					}
					else
					{
						ImGui::BeginDisabled();
						ImGui::SmallButton("Pause");
						ImGui::EndDisabled();
					}

					ImGui::SameLine();

					if (ImGui::SmallButton("Stop"))
					{
						m_moduleMusic->stopEvent(playingSound.playingID);
					}

					ImGui::PopID();
				}

				ImGui::EndTable();
			}
		}
	}

	ImGui::EndChild();
}