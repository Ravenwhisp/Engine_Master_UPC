#pragma once
#include "ScriptAPI.h"
#include "ParticleLifecycle.h"

#include <vector>

class DeathParticles;

// Visuals for Death's taunt: 3D chain links that fly out, grab enemies, drag them in and reel back.
// Owned by DeathParticles. All prefabs and settings are the "Taunt Chain ..." fields on DeathParticles,
// so they are assigned in the inspector there; this class only reads them.
class DeathTauntChains
{
public:
    explicit DeathTauntChains(DeathParticles& owner);

    void update(float deltaTime);
    void shutdown();

    void launch(const std::vector<GameObject*>& targets, float travelTime);
    void cancel();

    void onPullStarted(GameObject* enemy);
    void onPullFinished(GameObject* enemy);

private:
    enum class State
    {
        Idle,
        Shooting,
        Latched,
        Retracting
    };

    enum class PullState
    {
        None,
        Pulling,
        Finished
    };

    struct Chain
    {
        std::vector<GameObject*> links;
        std::vector<bool> linkShown;

        GameObject* target = nullptr;
        State state = State::Idle;
        PullState pull = PullState::None;
        float timer = 0.0f;
        float travelTime = 0.15f;
        float phase = 0.0f;
        bool grabBurstPlayed = false;
        Vector3 tip = Vector3::Zero;
        Vector3 retractFrom = Vector3::Zero;
    };

    void ensurePool();
    void destroyPool();

    void updateChain(Chain& chain, float deltaTime, const Vector3& start);
    void layout(Chain& chain, const Vector3& start, float whip);
    void hide(Chain& chain, const Vector3& parkPosition);
    void startRetract(Chain& chain);
    Chain* findChain(GameObject* enemy);

    Vector3 getStart() const;
    Vector3 getContactPoint(GameObject* target) const;
    Vector3 getEnemyCenter(GameObject* target) const;
    bool isTargetValid(GameObject* target) const;

    DeathParticles& m_owner;

    std::vector<Chain> m_chains;
    GameObject* m_root = nullptr;
    Transform* m_handBone = nullptr;
    bool m_poolBuilt = false;
    bool m_rootSeenInScene = false;
    float m_clock = 0.0f;

    ParticleLifecycle::TimedParticleTracker m_oneShots;
};
