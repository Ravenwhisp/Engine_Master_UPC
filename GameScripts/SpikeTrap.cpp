#include "pch.h"
#include "SpikeTrap.h"
#include "PlayerDamageable.h"
#include "EnvironmentSound.h"

IMPLEMENT_SCRIPT_FIELDS(SpikeTrap,
    SERIALIZED_BOOL(alternativeMode, "Alternative Mode"),
    SERIALIZED_FLOAT(a_duration, "Active Duration", 0.0f, 50.0f, 0.1f),
    SERIALIZED_FLOAT(p_duration, "Preparing Duration", 0.0f, 50.0f, 0.1f),
    SERIALIZED_FLOAT(startPositionY, "Start Position Y", -10.0f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(waitPositionY, "Wait Position Y", -10.0f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(activePositionY, "Active Position Y", -10.0f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(trapDamage, "Trap Damage", 0.0f, 1000.0f, 1.0f),
    SERIALIZED_COMPONENT_REF(m_spikeShineT, "Spike Shine Particle", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_spectralAuraT, "Spectral Aura Particle", ComponentType::TRANSFORM)
)

SpikeTrap::SpikeTrap(GameObject* owner)
    : Script(owner)
{
}

void SpikeTrap::Start()
{
    owner = getOwner();
    ownerTransform = GameObjectAPI::getTransform(owner);

    m_normalSpike = TransformAPI::findChildByName(ownerTransform, "Normal");
	m_spectralSpike = TransformAPI::findChildByName(ownerTransform, "Spectral");

    spikeType = alternativeMode ? 1 : 0;

    currentTime = 0.0f;
    state = WAIT;
    damagedPlayers.clear();
}

void SpikeTrap::Update()
{
    float dt = Time::getDeltaTime();
    currentTime += dt;
	
    const auto previousState = state;

    switch (state)
    {
        case SpikeTrap::WAIT:
            
            if (spikeType == 0)
            {
				normalSpikePosition.y = waitPositionY;
                TransformAPI::setPosition(m_normalSpike, normalSpikePosition);
            }
            else if (spikeType == 1)
            {
				spectralSpikePosition.y = waitPositionY;
				TransformAPI::setPosition(m_spectralSpike, spectralSpikePosition);
            }

            if (currentTime >= p_duration && spikeType == 0)
            {
                normalSpikePosition.y = activePositionY;
				TransformAPI::setPosition(m_normalSpike, normalSpikePosition);
                state = ACTIVE;
                currentTime = 0.0f;
				addEffect(0);
            }
			else if(currentTime >= p_duration && spikeType == 1)
            {
				spectralSpikePosition.y = activePositionY;
                TransformAPI::setPosition(m_spectralSpike, spectralSpikePosition);
                state = ACTIVE;
				currentTime = 0.0f;
				addEffect(1);
            }
            break;

        case SpikeTrap::ACTIVE:
			triggerBoxDamage();
            if (currentTime >= a_duration && spikeType == 0)
            {
				normalSpikePosition.y = startPositionY;
				spectralSpikePosition.y = waitPositionY;
				TransformAPI::setPosition(m_normalSpike, normalSpikePosition);
                TransformAPI::setPosition(m_spectralSpike, spectralSpikePosition);
				spikeType = 1;
                state = WAIT;
                currentTime = 0.0f;
				damagedPlayers.clear();
				removeEffect(0);
            }
			else if (currentTime >= a_duration && spikeType == 1)
			{
				normalSpikePosition.y = waitPositionY;
				spectralSpikePosition.y = startPositionY;
                TransformAPI::setPosition(m_normalSpike, normalSpikePosition);
                TransformAPI::setPosition(m_spectralSpike, spectralSpikePosition);
				spikeType = 0;
				state = WAIT;
				currentTime = 0.0f;
                damagedPlayers.clear();
				removeEffect(1);
            }
            break;

        default:
            break;
    }

    // Single hook for all 4 transition branches (normal/spectral × extend/retract).
    if (state != previousState)
    {
        if (state == ACTIVE)
        {
            EnvironmentSound::play(getOwner(), "Play_Environment_Extend_Spikes");
        }
        else if (state == WAIT)
        {
            EnvironmentSound::play(getOwner(), "Play_Environment_Retract_Spikes");
        }
    }

    }

bool SpikeTrap::containsPoint(const Vector3& triggerCenter, const Vector3& point) const
{
    const float halfX = m_xWidth * 0.5f;
    const float halfZ = m_zWidth * 0.5f;

    return point.x >= triggerCenter.x - halfX &&
        point.x <= triggerCenter.x + halfX &&
        point.z >= triggerCenter.z - halfZ &&
        point.z <= triggerCenter.z + halfZ;
}

void SpikeTrap::TrapLoop()
{
    
        
}

void SpikeTrap::damagePlayer(GameObject* player)
{
    // Skip if this player was already damaged
    if (damagedPlayers.count(player)) return;

    PlayerDamageable* damageable = GameObjectAPI::findScript<PlayerDamageable>(player);
    if (damageable)
    {
        damageable->takeDamage(trapDamage);
        damagedPlayers.insert(player);
    }
}

void SpikeTrap::triggerBoxDamage()
{
    GameObject* owner = getOwner();
    Transform* ownerTransform = GameObjectAPI::getTransform(owner);
    const Vector3 trapPosition = TransformAPI::getGlobalPosition(ownerTransform);
    std::vector<GameObject*> playersInScene = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER);
    for (GameObject* player : playersInScene)
    {
		const char* name = GameObjectAPI::getName(player);
        
        if(name && strcmp(name, "Lyriel") == 0 && spikeType == 0)
        {
            Transform* playerTransform = GameObjectAPI::getTransform(player);
            const Vector3 playerPosition = TransformAPI::getGlobalPosition(playerTransform);
            if (containsPoint(trapPosition, playerPosition))
            {
                damagePlayer(player);
            }
            else
            {
                damagedPlayers.erase(player);
            }
		}
        if(name && strcmp(name, "Death") == 0 && spikeType == 1)
        {
            Transform* playerTransform = GameObjectAPI::getTransform(player);
            const Vector3 playerPosition = TransformAPI::getGlobalPosition(playerTransform);
            if (containsPoint(trapPosition, playerPosition))
            {
                damagePlayer(player);
            }
            else
            {
                damagedPlayers.erase(player);
            }
		}
        
    }
}

void SpikeTrap::addEffect(int type)
{
    if (type == 0)
    {
        GameObjectAPI::setActive(ComponentAPI::getOwner(m_spikeShineT.getReferencedComponent()), true);
    }
    else if (type == 1)
    {
        GameObjectAPI::setActive(ComponentAPI::getOwner(m_spectralAuraT.getReferencedComponent()), true);
    }
}

void SpikeTrap::removeEffect(int type)
{
    if (type == 0)
    {
        GameObjectAPI::setActive(ComponentAPI::getOwner(m_spikeShineT.getReferencedComponent()), false);
    }
    else if (type == 1)
    {
        GameObjectAPI::setActive(ComponentAPI::getOwner(m_spectralAuraT.getReferencedComponent()), false);
    }
}


IMPLEMENT_SCRIPT(SpikeTrap)