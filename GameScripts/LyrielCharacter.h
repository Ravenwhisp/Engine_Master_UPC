#pragma once

#include "CharacterBase.h"

class ArrowPool;
class LyrielDash;
class LyrielArrowVolley;
class LyrielSound;
class PlayerMovement;
class LyrielConfig;

class LyrielCharacter : public CharacterBase
{
    DECLARE_SCRIPT(LyrielCharacter)

public:
    explicit LyrielCharacter(GameObject* owner);

    void Start()  override;
    void Update() override;
    ScriptFieldList getExposedFields() const override;

    ArrowPool*   getArrowPool() const { return m_arrowPool; }
    LyrielSound* getSound()     const { return m_sound; }
    LyrielConfig* getConfig() const { return m_config; }

    // Called by Lyriel's offensive scripts after they exploit a Phase 3 mark.
    // Grants the two design-defined rewards: +1 dash charge and -20% of base
    // Arrow Volley cooldown.
    void onMarkExploited();

public:
    std::string m_arrowSpawnChildName = "ArrowSpawn";

private:
    ArrowPool*         m_arrowPool   = nullptr;
    LyrielDash*        m_dash        = nullptr;
    LyrielArrowVolley* m_arrowVolley = nullptr;
    LyrielSound*       m_sound       = nullptr;
    PlayerMovement*    m_movement    = nullptr;
    LyrielConfig*      m_config = nullptr;
};