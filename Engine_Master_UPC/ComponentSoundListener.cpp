#include "Globals.h"
#include "ComponentSoundListener.h"
#include "JsonArchive.h"

#include "Application.h"
#include "ModuleMusic.h"

#include "GameObject.h"
#include "Transform.h"

#include <memory>

ComponentSoundListener::ComponentSoundListener(UID id, GameObject* gameObject) : Component(id, ComponentType::SOUND_LISTENER, gameObject)
{
	m_moduleMusic = app->getModuleMusic();
	m_audioGameObjectID = static_cast<uint64_t>(getOwner()->GetID());
}

ComponentSoundListener::~ComponentSoundListener() = default;

bool ComponentSoundListener::init()
{
	if (!m_moduleMusic || !getOwner())
	{
		return false;
	}

	return m_moduleMusic->registerListener(m_audioGameObjectID, getOwner()->GetName().c_str());
}

void ComponentSoundListener::update()
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

bool ComponentSoundListener::cleanUp()
{
	if (m_moduleMusic)
	{
		m_moduleMusic->unregisterListener(m_audioGameObjectID);
	}

	return true;
}

std::unique_ptr<Component> ComponentSoundListener::clone(GameObject* newOwner) const
{
	auto cloned = std::make_unique<ComponentSoundListener>(m_uuid, newOwner);

	cloned->setActive(isActive());

	return cloned;
}

void ComponentSoundListener::serialize(IArchive& archive)
{
	if (archive.mode() == ArchiveMode::Output)
	{
		uint64_t uid = m_uuid;
		archive.serialize(uid, "UID");
		uint32_t type = static_cast<uint32_t>(ComponentType::SOUND_LISTENER);
		archive.serialize(type, "ComponentType");
	}

	bool active = isActive();
	archive.serialize(active, "Active");
	if (archive.mode() == ArchiveMode::Input)
		setActive(active);
}