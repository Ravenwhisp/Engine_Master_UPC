#include "pch.h"
#include "SpikeTrap.h"
#include "PlayerDamageable.h"
#include "EnvironmentSound.h"
#include "ParticleLifecycle.h"

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

    {
        SCRIPT_PROFILE_SCOPE("Start: find children");
        m_normalSpike = TransformAPI::findChildByName(ownerTransform, "Normal");
	    m_spectralSpike = TransformAPI::findChildByName(ownerTransform, "Spectral");
    }

    {
        SCRIPT_PROFILE_SCOPE("Start: cache players");
        const std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER);
        for (GameObject* player : players)
        {
            const char* name = GameObjectAPI::getName(player);
            if (!name)
            {
                continue;
            }

            if (strcmp(name, "Lyriel") == 0)
            {
                m_lyriel = player;
            }
            else if (strcmp(name, "Death") == 0)
            {
                m_death = player;
            }
        }
    }

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
            {
                SCRIPT_PROFILE_SCOPE("WAIT: set position");
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
            }

            if (currentTime >= p_duration && spikeType == 0)
            {
                {
                    SCRIPT_PROFILE_SCOPE("Activate: set position");
                    normalSpikePosition.y = activePositionY;
				    TransformAPI::setPosition(m_normalSpike, normalSpikePosition);
                }
                state = ACTIVE;
                currentTime = 0.0f;
				{
                    SCRIPT_PROFILE_SCOPE("Activate: particles");
                    addEffect(0);
                }
            }
			else if(currentTime >= p_duration && spikeType == 1)
            {
                {
                    SCRIPT_PROFILE_SCOPE("Activate: set position");
				    spectralSpikePosition.y = activePositionY;
                    TransformAPI::setPosition(m_spectralSpike, spectralSpikePosition);
                }
                state = ACTIVE;
				currentTime = 0.0f;
				{
                    SCRIPT_PROFILE_SCOPE("Activate: particles");
                    addEffect(1);
                }
            }
            break;

        case SpikeTrap::ACTIVE:
			{
                SCRIPT_PROFILE_SCOPE("ACTIVE: trigger damage");
                triggerBoxDamage();
            }
            if (currentTime >= a_duration && spikeType == 0)
            {
				{
                    SCRIPT_PROFILE_SCOPE("Deactivate: set positions");
                    normalSpikePosition.y = startPositionY;
				    spectralSpikePosition.y = waitPositionY;
				    TransformAPI::setPosition(m_normalSpike, normalSpikePosition);
                    TransformAPI::setPosition(m_spectralSpike, spectralSpikePosition);
                }
				spikeType = 1;
                state = WAIT;
                currentTime = 0.0f;
				damagedPlayers.clear();
				{
                    SCRIPT_PROFILE_SCOPE("Deactivate: particles");
                    removeEffect(0);
                }
            }
			else if (currentTime >= a_duration && spikeType == 1)
			{
				{
                    SCRIPT_PROFILE_SCOPE("Deactivate: set positions");
                    normalSpikePosition.y = waitPositionY;
				    spectralSpikePosition.y = startPositionY;
                    TransformAPI::setPosition(m_normalSpike, normalSpikePosition);
                    TransformAPI::setPosition(m_spectralSpike, spectralSpikePosition);
                }
				spikeType = 0;
				state = WAIT;
				currentTime = 0.0f;
                damagedPlayers.clear();
				{
                    SCRIPT_PROFILE_SCOPE("Deactivate: particles");
                    removeEffect(1);
                }
			}
            break;

        default:
            break;
    }

    // Single hook for all 4 transition branches (normal/spectral × extend/retract).
    if (state != previousState)
    {
        SCRIPT_PROFILE_SCOPE("Transition: sound");
        if (state == ACTIVE)
        {
            EnvironmentSound::playGrouped(
                getOwner(), "Play_Environment_Extend_Spikes", "SpikeTraps", 100);
        }
        else if (state == WAIT)
        {
            EnvironmentSound::playGrouped(
                getOwner(), "Play_Environment_Retract_Spikes", "SpikeTraps", 100);
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

    PlayerDamageable* damageable = nullptr;
    {
        SCRIPT_PROFILE_SCOPE("Damage: find PlayerDamageable");
        damageable = GameObjectAPI::findScript<PlayerDamageable>(player);
    }
    if (damageable)
    {
        {
            SCRIPT_PROFILE_SCOPE("Damage: apply player damage");
            damageable->takeDamage(trapDamage);
        }
        damagedPlayers.insert(player);
    }
}

void SpikeTrap::triggerBoxDamage()
{
    GameObject* owner = getOwner();
    Vector3 trapPosition;
    {
        SCRIPT_PROFILE_SCOPE("Trigger: get trap position");
        Transform* ownerTransform = GameObjectAPI::getTransform(owner);
        trapPosition = TransformAPI::getGlobalPosition(ownerTransform);
    }

    GameObject* player = spikeType == 0 ? m_lyriel : m_death;
    if (!player)
    {
        return;
    }

    {
        SCRIPT_PROFILE_SCOPE("Trigger: collision/damage loop");
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

void SpikeTrap::addEffect(int type)
{
    Transform* effectTransform = nullptr;

    if (type == 0)
    {
        effectTransform = m_spikeShineT.getReferencedComponent();
    }
    else if (type == 1)
    {
        effectTransform = m_spectralAuraT.getReferencedComponent();
    }

    if (effectTransform == nullptr)
    {
        return;
    }

    GameObject* effectObject = ComponentAPI::getOwner(effectTransform);
    ParticleLifecycle::activate(effectObject);
}

void SpikeTrap::removeEffect(int type)
{
    Transform* effectTransform = nullptr;

    if (type == 0)
    {
        effectTransform = m_spikeShineT.getReferencedComponent();
    }
    else if (type == 1)
    {
        effectTransform = m_spectralAuraT.getReferencedComponent();
    }

    if (effectTransform == nullptr)
    {
        return;
    }

    GameObject* effectObject = ComponentAPI::getOwner(effectTransform);
    ParticleLifecycle::deactivate(effectObject);
}


IMPLEMENT_SCRIPT(SpikeTrap)
