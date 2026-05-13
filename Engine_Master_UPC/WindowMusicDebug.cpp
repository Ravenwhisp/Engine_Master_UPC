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
	if (!m_moduleMusic)
	{
		return;
	}

	const std::vector<WwiseBank>& banks = m_moduleMusic->getBankList();

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

	ImGui::Separator();

	if (ImGui::CollapsingHeader("Playing Sounds"))
	{
		const std::vector<PlayingSound>& playingSounds = m_moduleMusic->getPlayingSounds();

		if (playingSounds.empty())
		{
			ImGui::Text("No playing sounds");
			return;
		}

		for (const PlayingSound& playingSound : playingSounds)
		{
			ImGui::Text("Bank: %s", playingSound.bankName.c_str());
			ImGui::Text("Event: %s", playingSound.eventName.c_str());
			ImGui::Text("Playing ID: %u", playingSound.playingID);
			ImGui::Separator();
		}
	}
}