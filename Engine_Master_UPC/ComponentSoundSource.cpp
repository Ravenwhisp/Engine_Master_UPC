#include "Globals.h"
#include "ComponentSoundSource.h"
#include "JsonArchive.h"

#include "Application.h"
#include "ModuleMusic.h"

#include "GameObject.h"
#include "Transform.h"

#include <memory>

ComponentSoundSource::ComponentSoundSource(UID id, GameObject* gameObject) : Component(id, ComponentType::SOUND_SOURCE, gameObject)
{
	m_moduleMusic = app->getModuleMusic();
	m_audioGameObjectID = static_cast<uint64_t>(getOwner()->GetID());
}

ComponentSoundSource::~ComponentSoundSource() = default;

bool ComponentSoundSource::init()
{
	if (!m_moduleMusic || !getOwner())
	{
		return false;
	}

	return m_moduleMusic->registerAudioGameObject(m_audioGameObjectID, getOwner()->GetName().c_str());
}

void ComponentSoundSource::update()
{
	if (!m_moduleMusic || !getOwner())
	{
		return;
	}

	Transform* transform = getOwner()->GetTransform();

	if (!transform)
	{
		return;
	}

	const Matrix& globalMatrix = transform->getGlobalMatrix();

	const Vector3 position(globalMatrix._41, globalMatrix._42, globalMatrix._43);

	const Vector3 forward = transform->getForward();
	const Vector3 up = transform->getUp();

	m_moduleMusic->setAudioGameObjectTransform(m_audioGameObjectID, position, forward, up);
}

bool ComponentSoundSource::cleanUp()
{
	if (m_moduleMusic)
	{
		m_moduleMusic->unregisterAudioGameObject(m_audioGameObjectID);
	}

	return true;
}

uint32_t ComponentSoundSource::postEvent(const char* bankName, const char* eventName)
{
	if (!m_moduleMusic)
	{
		return 0;
	}

	return m_moduleMusic->postEvent(bankName, eventName, m_audioGameObjectID);
}

void ComponentSoundSource::stopEvent(uint32_t playingID)
{
	if (m_moduleMusic)
	{
		m_moduleMusic->stopEvent(playingID);
	}
}

void ComponentSoundSource::pauseEvent(uint32_t playingID)
{
	if (m_moduleMusic)
	{
		m_moduleMusic->pauseEvent(playingID);
	}
}

void ComponentSoundSource::resumeEvent(uint32_t playingID)
{
	if (m_moduleMusic)
	{
		m_moduleMusic->resumeEvent(playingID);
	}
}

std::unique_ptr<Component> ComponentSoundSource::clone(GameObject* newOwner) const
{
	auto cloned = std::make_unique<ComponentSoundSource>(m_uuid, newOwner);

	cloned->setActive(isActive());

	return cloned;
}

void ComponentSoundSource::drawUi()
{
	if (!m_moduleMusic)
	{
		return;
	}

	const std::vector<WwiseBank>& banks = m_moduleMusic->getBankList();

	static int selectedBankIndex = -1;
	static int selectedEventIndex = -1;

	if (banks.empty())
	{
		ImGui::TextDisabled("No audio banks found");
		return;
	}

	if (selectedBankIndex < 0 || selectedBankIndex >= static_cast<int>(banks.size()))
	{
		selectedBankIndex = 0;
		selectedEventIndex = -1;
	}

	const WwiseBank& selectedBank = banks[selectedBankIndex];

	if (ImGui::BeginCombo("Bank", selectedBank.getName().c_str()))
	{
		for (int i = 0; i < static_cast<int>(banks.size()); ++i)
		{
			const WwiseBank& bank = banks[i];

			std::string label = bank.getName();

			if (!bank.isLoaded())
			{
				label += " [Unloaded]";
			}

			const bool selected = selectedBankIndex == i;

			if (ImGui::Selectable(label.c_str(), selected))
			{
				selectedBankIndex = i;
				selectedEventIndex = -1;
			}

			if (selected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}

	if (!selectedBank.isLoaded())
	{
		ImGui::TextDisabled("Selected bank is not loaded");
		return;
	}

	const std::vector<WwiseEvent>& events = selectedBank.getEvents();

	if (events.empty())
	{
		ImGui::TextDisabled("Selected bank has no events");
		return;
	}

	if (selectedEventIndex < 0 || selectedEventIndex >= static_cast<int>(events.size()))
	{
		selectedEventIndex = 0;
	}

	const WwiseEvent& selectedEvent = events[selectedEventIndex];

	if (ImGui::BeginCombo("Event", selectedEvent.name.c_str()))
	{
		for (int i = 0; i < static_cast<int>(events.size()); ++i)
		{
			const WwiseEvent& event = events[i];
			const bool selected = selectedEventIndex == i;

			if (ImGui::Selectable(event.name.c_str(), selected))
			{
				selectedEventIndex = i;
			}

			if (selected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}

	ImGui::Spacing();

	if (ImGui::Button("Test Event", ImVec2(-1.0f, 0.0f)))
	{
		postEvent(selectedBank.getName().c_str(), selectedEvent.name.c_str());
	}
}

void ComponentSoundSource::serialize(IArchive& archive)
{
	if (archive.mode() == ArchiveMode::Output)
	{
		uint64_t uid = m_uuid;
		archive.serialize(uid, "UID");
		uint32_t type = static_cast<uint32_t>(ComponentType::SOUND_SOURCE);
		archive.serialize(type, "ComponentType");
	}

	bool active = isActive();
	archive.serialize(active, "Active");
	if (archive.mode() == ArchiveMode::Input)
		setActive(active);
}