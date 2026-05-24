#pragma once

#include "CharacterBase.h"

class ArrowPool;
class LyrielDash;
class LyrielArrowVolley;

class LyrielCharacter : public CharacterBase
{
    DECLARE_SCRIPT(LyrielCharacter)

public:
    explicit LyrielCharacter(GameObject* owner);

    void Start() override;
    ScriptFieldList getExposedFields() const override;

    ArrowPool* getArrowPool() const { return m_arrowPool; }

    // Called by Lyriel's offensive scripts after they exploit a Phase 3 mark.
    // Grants the two design-defined rewards: +1 dash charge and -20% of base
    // Arrow Volley cooldown.
    void onMarkExploited();

public:
    std::string m_arrowSpawnChildName = "ArrowSpawn";
    float       m_volleyCooldownReductionPerExploit = 0.20f;

private:
    ArrowPool*         m_arrowPool   = nullptr;
    LyrielDash*        m_dash        = nullptr;
    LyrielArrowVolley* m_arrowVolley = nullptr;
};