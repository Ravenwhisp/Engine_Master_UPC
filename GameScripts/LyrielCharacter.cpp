#include "pch.h"
#include "LyrielCharacter.h"
#include "ArrowPool.h"
#include "LyrielDash.h"
#include "LyrielArrowVolley.h"
#include "LyrielSound.h"
#include "PlayerMovement.h"
#include "LyrielConfig.h"

IMPLEMENT_SCRIPT_FIELDS(LyrielCharacter,
    SERIALIZED_STRING(m_arrowSpawnChildName, "Arrow Spawn Child Name")
)

LyrielCharacter::LyrielCharacter(GameObject* owner)
    : CharacterBase(owner)
{
}

void LyrielCharacter::Start()
{
    CharacterBase::Start();

    m_arrowPool     = GameObjectAPI::findScript<ArrowPool>(getOwner());
    m_dash          = GameObjectAPI::findScript<LyrielDash>(getOwner());
    m_arrowVolley   = GameObjectAPI::findScript<LyrielArrowVolley>(getOwner());
    m_sound         = GameObjectAPI::findScript<LyrielSound>(getOwner());
    m_movement      = GameObjectAPI::findScript<PlayerMovement>(getOwner());
    m_config        = GameObjectAPI::findScript<LyrielConfig>(getOwner());

    if (m_arrowPool == nullptr)
    {
        Debug::log("[LyrielCharacter] ArrowPool not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_dash == nullptr)
    {
        Debug::log("[LyrielCharacter] LyrielDash not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_arrowVolley == nullptr)
    {
        Debug::log("[LyrielCharacter] LyrielArrowVolley not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_sound == nullptr)
    {
        Debug::log("[LyrielCharacter] LyrielSound not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_movement == nullptr)
    {
        Debug::log("[LyrielCharacter] PlayerMovement not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_config == nullptr)
    {
        Debug::log("[LyrielCharacter] LyrielConfig not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }
}

void LyrielCharacter::Update()
{
    if (m_sound == nullptr)
    {
        return;
    }

    if (isDowned())
    {
        m_sound->stopAllLoops();
        return;
    }

    if (m_movement != nullptr)
    {
        m_sound->setFootstepsActive(m_movement->isMoving());
    }
}

void LyrielCharacter::onMarkExploited()
{
    if (m_dash != nullptr)
    {
        m_dash->recoverCharge();
    }

    if (m_arrowVolley != nullptr && m_config != nullptr)
    {
        m_arrowVolley->reduceCooldown(m_config->m_volleyCooldownReductionPerExploit);
    }
}

IMPLEMENT_SCRIPT(LyrielCharacter)