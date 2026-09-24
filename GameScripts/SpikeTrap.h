#pragma once

#include "ScriptAPI.h"

#include <unordered_set>

class SpikeTrap : public Script
{
    DECLARE_SCRIPT(SpikeTrap)

public:
    explicit SpikeTrap(GameObject* owner);

    void Start() override;
    void Update() override;
    void OnTriggerEnter(GameObject* gameObject) override;
    void OnTriggerExit(GameObject* gameObject) override;

    FieldList getExposedFields() const override;

    enum TrapState
    {
        WAIT,
        EXTENDING,
        ACTIVE,
        RETRACTING
    };

    float a_duration = 2.0f;
    float p_duration = 1.0f;
    float extensionDuration = 0.35f;
    float retractionDuration = 0.55f;

    float trapDamage = 20.0f;
    bool alternativeMode = false;

    // Hidden, telegraph, and fully dangerous local heights.
    float startPositionY = -1.0f;
    float waitPositionY = -0.7f;
    float activePositionY = 0.0f;

    ComponentRef<Transform> m_spikeShineT;
    ComponentRef<Transform> m_spectralAuraT;

private:
    void beginExtension();
    void updateExtension();
    void beginRetraction();
    void updateRetraction();

    void setSpikeHeight(int type, float height);
    float getAnimationProgress(float duration) const;
    static float smoothStep(float t);

    bool isCurrentTarget(GameObject* player) const;
    void damagePlayersInside();
    void damagePlayer(GameObject* player);

    void addEffect(int type);
    void removeEffect(int type);

    int spikeType = 0; // 0: normal/Lyriel, 1: spectral/Death
    float currentTime = 0.0f;
    TrapState state = WAIT;

    Transform* m_normalSpike = nullptr;
    Transform* m_spectralSpike = nullptr;

    GameObject* m_lyriel = nullptr;
    GameObject* m_death = nullptr;

    std::unordered_set<GameObject*> playersInside;
    std::unordered_set<GameObject*> damagedPlayers;
};
