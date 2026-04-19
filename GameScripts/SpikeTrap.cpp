#include "pch.h"
#include "SpikeTrap.h"
#include "PlayerDamageable.h"

static const ScriptFieldInfo myScriptFields[] =
{
	{ "Alternative Mode", ScriptFieldType::Bool, offsetof(SpikeTrap, alternativeMode) },
    { "Active Duration" , ScriptFieldType::Float, offsetof(SpikeTrap, a_duration), {0.0f, 50.0f, 0.1f } },
	{ "Preparing Duration" , ScriptFieldType::Float, offsetof(SpikeTrap, p_duration), {0.0f, 50.0f, 0.1f } },
    { "Start Position Y", ScriptFieldType::Float, offsetof(SpikeTrap, startPositionY), { -10.0f, 10.0f, 0.1f } },
    { "Wait Position Y", ScriptFieldType::Float, offsetof(SpikeTrap, waitPositionY), { -10.0f, 10.0f, 0.1f } },
	{ "Active Position Y", ScriptFieldType::Float, offsetof(SpikeTrap, activePositionY), { -10.0f, 10.0f, 0.1f } },
	{ "Trap Damage", ScriptFieldType::Float, offsetof(SpikeTrap, trapDamage), { 0.0f, 1000.0f, 1.0f } },
};

IMPLEMENT_SCRIPT_FIELDS(SpikeTrap, myScriptFields)

SpikeTrap::SpikeTrap(GameObject* owner)
    : Script(owner)
{
}

void SpikeTrap::Start()
{

    if (alternativeMode)
    {
		spikeType = 1;
    }

}

void SpikeTrap::Update()
{
    currentTime += dt;
	m_normalSpike = TransformAPI::findChildByName(ownerTransform, "Normal");
	m_spectralSpike = TransformAPI::findChildByName(ownerTransform, "Spectral");

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

            }
			else if(currentTime >= p_duration && spikeType == 1)
            {
				spectralSpikePosition.y = activePositionY;
                TransformAPI::setPosition(m_spectralSpike, spectralSpikePosition);
                state = ACTIVE;
				currentTime = 0.0f;
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
            }
            break;

        default:
            break;
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

    Script* damageableScript = GameObjectAPI::getScript(player, "PlayerDamageable");
    PlayerDamageable* damageable = dynamic_cast<PlayerDamageable*>(damageableScript);
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
            const Vector3 playerPosition = TransformAPI::getPosition(playerTransform);
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
            const Vector3 playerPosition = TransformAPI::getPosition(playerTransform);
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


IMPLEMENT_SCRIPT(SpikeTrap)