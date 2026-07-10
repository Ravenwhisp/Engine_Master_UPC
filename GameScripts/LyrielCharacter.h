#pragma once

#include "CharacterBase.h"

class ProjectilePool;
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
    FieldList getExposedFields() const override;

    ProjectilePool*   getArrowPool() const { return m_arrowPool; }
    LyrielSound* getSound()     const { return m_sound; }
    // Called by Lyriel's offensive scripts after they exploit a Phase 3 mark.
    // Grants the two design-defined rewards: +1 dash charge and -20% of base
    // Arrow Volley cooldown.
    void onMarkExploited();

public:
    std::string m_arrowSpawnChildName = "ArrowSpawn";

private:
    ProjectilePool*    m_arrowPool   = nullptr;
    LyrielDash*        m_dash        = nullptr;
    LyrielSound*       m_sound       = nullptr;
    PlayerMovement*    m_movement    = nullptr;
    AssetRef<LyrielConfig> m_config;
};