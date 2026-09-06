#include "pch.h"
#include "SpikeTrap.h"
#include "PlayerDamageable.h"
#include "EnvironmentSound.h"
#include "ParticleLifecycle.h"

#include <chrono>

namespace
{
    class SpikeTrapProfileScope
    {
    public:
        SpikeTrapProfileScope(GameObject* owner, const char* scopeName)
            : m_owner(owner), m_scopeName(scopeName), m_enabled(ScriptProfilerAPI::isEnabled())
        {
            if (m_enabled)
            {
                m_start = Clock::now();
            }
        }

        ~SpikeTrapProfileScope()
        {
            if (!m_enabled)
            {
                return;
            }

            const float elapsedMs = std::chrono::duration<float, std::milli>(Clock::now() - m_start).count();
            ScriptProfilerAPI::recordScope("SpikeTrap", m_scopeName, m_owner, elapsedMs);
        }

    private:
        using Clock = std::chrono::steady_clock;

        GameObject* m_owner = nullptr;
        const char* m_scopeName = nullptr;
        bool m_enabled = false;
        Clock::time_point m_start{};
    };
}

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
        SpikeTrapProfileScope profile(owner, "Start: find children");
        m_normalSpike = TransformAPI::findChildByName(ownerTransform, "Normal");
	    m_spectralSpike = TransformAPI::findChildByName(ownerTransform, "Spectral");
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
                SpikeTrapProfileScope profile(owner, "WAIT: set position");
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
                    SpikeTrapProfileScope profile(owner, "Activate: set position");
                    normalSpikePosition.y = activePositionY;
				    TransformAPI::setPosition(m_normalSpike, normalSpikePosition);
                }
                state = ACTIVE;
                currentTime = 0.0f;
				{
                    SpikeTrapProfileScope profile(owner, "Activate: particles");
                    addEffect(0);
                }
            }
			else if(currentTime >= p_duration && spikeType == 1)
            {
                {
                    SpikeTrapProfileScope profile(owner, "Activate: set position");
				    spectralSpikePosition.y = activePositionY;
                    TransformAPI::setPosition(m_spectralSpike, spectralSpikePosition);
                }
                state = ACTIVE;
				currentTime = 0.0f;
				{
                    SpikeTrapProfileScope profile(owner, "Activate: particles");
                    addEffect(1);
                }
            }
            break;

        case SpikeTrap::ACTIVE:
			{
                SpikeTrapProfileScope profile(owner, "ACTIVE: trigger damage");
                triggerBoxDamage();
            }
            if (currentTime >= a_duration && spikeType == 0)
            {
				{
                    SpikeTrapProfileScope profile(owner, "Deactivate: set positions");
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
                    SpikeTrapProfileScope profile(owner, "Deactivate: particles");
                    removeEffect(0);
                }
            }
			else if (currentTime >= a_duration && spikeType == 1)
			{
				{
                    SpikeTrapProfileScope profile(owner, "Deactivate: set positions");
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
                    SpikeTrapProfileScope profile(owner, "Deactivate: particles");
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
        SpikeTrapProfileScope profile(owner, "Transition: sound");
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

    PlayerDamageable* damageable = nullptr;
    {
        SpikeTrapProfileScope profile(owner, "Damage: find PlayerDamageable");
        damageable = GameObjectAPI::findScript<PlayerDamageable>(player);
    }
    if (damageable)
    {
        {
            SpikeTrapProfileScope profile(owner, "Damage: apply player damage");
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
        SpikeTrapProfileScope profile(owner, "Trigger: get trap position");
        Transform* ownerTransform = GameObjectAPI::getTransform(owner);
        trapPosition = TransformAPI::getGlobalPosition(ownerTransform);
    }
    std::vector<GameObject*> playersInScene;
    {
        SpikeTrapProfileScope profile(owner, "Trigger: find players by tag");
        playersInScene = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER);
    }
    {
        SpikeTrapProfileScope profile(owner, "Trigger: collision/damage loop");
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
