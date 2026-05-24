#include "pch.h"
#include "LyrielCharacter.h"
#include "ArrowPool.h"
#include "LyrielDash.h"
#include "LyrielArrowVolley.h"

IMPLEMENT_SCRIPT_FIELDS(LyrielCharacter,
    SERIALIZED_STRING(m_arrowSpawnChildName, "Arrow Spawn Child Name"),
    SERIALIZED_FLOAT(m_volleyCooldownReductionPerExploit, "Volley CD Reduction Per Exploit", 0.0f, 1.0f, 0.05f)
)

LyrielCharacter::LyrielCharacter(GameObject* owner)
    : CharacterBase(owner)
{
}

void LyrielCharacter::Start()
{
    CharacterBase::Start();

    m_arrowPool   = GameObjectAPI::findScript<ArrowPool>(getOwner());
    m_dash        = GameObjectAPI::findScript<LyrielDash>(getOwner());
    m_arrowVolley = GameObjectAPI::findScript<LyrielArrowVolley>(getOwner());

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
}

void LyrielCharacter::onMarkExploited()
{
    if (m_dash != nullptr)
        m_dash->recoverCharge();

    if (m_arrowVolley != nullptr)
        m_arrowVolley->reduceCooldown(m_volleyCooldownReductionPerExploit);
}

IMPLEMENT_SCRIPT(LyrielCharacter)