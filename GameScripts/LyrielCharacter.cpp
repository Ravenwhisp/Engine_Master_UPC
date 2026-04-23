#include "pch.h"
#include "LyrielCharacter.h"
#include "ArrowPool.h"

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

    Script* arrowPoolScript = GameObjectAPI::getScript(getOwner(), "ArrowPool");
    m_arrowPool = static_cast<ArrowPool*>(arrowPoolScript);

    if (m_arrowPool == nullptr)
    {
        Debug::log("[LyrielCharacter] ArrowPool not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }
}

IMPLEMENT_SCRIPT(LyrielCharacter)