#pragma once

#include "EngineAPI.h"
#include "DestroyParticles.h"

#include <vector>

namespace ParticleLifecycle
{
    inline constexpr float kDefaultOneShotLifetime = 3.0f;
    inline constexpr float kDefaultPersistentDeactivateDelay = 1.0f;

    struct TimedParticleEntry
    {
        GameObject* instance = nullptr;
        float remainingSeconds = 0.0f;
        bool deactivateOnExpire = false;
    };

    inline void visitParticleSystems(GameObject* gameObject, void (*fn)(ParticleSystemComponent*))
    {
        if (gameObject == nullptr || fn == nullptr)
        {
            return;
        }

        ParticleSystemComponent* particleSystem = ParticleSystemAPI::getParticleSystemComponent(gameObject);
        if (particleSystem != nullptr)
        {
            fn(particleSystem);
        }

        Transform* transform = GameObjectAPI::getTransform(gameObject);
        if (transform == nullptr)
        {
            return;
        }

        const int childCount = TransformAPI::getChildCount(transform);
        for (int i = 0; i < childCount; ++i)
        {
            Transform* child = TransformAPI::getChild(transform, i);
            if (child == nullptr)
            {
                continue;
            }

            visitParticleSystems(ComponentAPI::getOwner(child), fn);
        }
    }

    inline void disableSelfDestruct(GameObject* gameObject)
    {
        if (gameObject == nullptr)
        {
            return;
        }

        DestroyParticles* destroyParticles = GameObjectAPI::findScript<DestroyParticles>(gameObject);
        if (destroyParticles != nullptr)
        {
            destroyParticles->setAutoDestroy(false);
        }

        Transform* transform = GameObjectAPI::getTransform(gameObject);
        if (transform == nullptr)
        {
            return;
        }

        const int childCount = TransformAPI::getChildCount(transform);
        for (int i = 0; i < childCount; ++i)
        {
            Transform* child = TransformAPI::getChild(transform, i);
            if (child == nullptr)
            {
                continue;
            }

            disableSelfDestruct(ComponentAPI::getOwner(child));
        }
    }

    inline void restart(GameObject* gameObject)
    {
        visitParticleSystems(gameObject, [](ParticleSystemComponent* particleSystem)
        {
            ParticleSystemAPI::reset(particleSystem);
            ParticleSystemAPI::play(particleSystem);
        });
    }

    inline void stop(GameObject* gameObject)
    {
        visitParticleSystems(gameObject, [](ParticleSystemComponent* particleSystem)
        {
            ParticleSystemAPI::stop(particleSystem);
        });
    }

    inline void activate(GameObject* gameObject)
    {
        if (gameObject == nullptr)
        {
            return;
        }

        GameObjectAPI::setActive(gameObject, true);
        restart(gameObject);
    }

    inline void deactivate(GameObject* gameObject)
    {
        if (gameObject == nullptr)
        {
            return;
        }

        stop(gameObject);
        GameObjectAPI::setActive(gameObject, false);
    }

    struct TimedParticleTracker
    {
        std::vector<TimedParticleEntry> entries;

        void update(float deltaTime)
        {
            for (size_t i = 0; i < entries.size();)
            {
                TimedParticleEntry& entry = entries[i];
                entry.remainingSeconds -= deltaTime;

                if (entry.remainingSeconds > 0.0f)
                {
                    ++i;
                    continue;
                }

                if (entry.instance != nullptr)
                {
                    if (entry.deactivateOnExpire)
                    {
                        deactivate(entry.instance);
                    }
                    else
                    {
                        GameObjectAPI::removeGameObject(entry.instance);
                    }
                }

                entries.erase(entries.begin() + static_cast<std::ptrdiff_t>(i));
            }
        }

        void scheduleDestroy(GameObject* instance, float lifetime)
        {
            if (instance == nullptr || lifetime <= 0.0f)
            {
                return;
            }

            TimedParticleEntry entry;
            entry.instance = instance;
            entry.remainingSeconds = lifetime;
            entry.deactivateOnExpire = false;
            entries.push_back(entry);
        }

        void scheduleDeactivate(GameObject* instance, float lifetime)
        {
            if (instance == nullptr || lifetime <= 0.0f)
            {
                return;
            }

            TimedParticleEntry entry;
            entry.instance = instance;
            entry.remainingSeconds = lifetime;
            entry.deactivateOnExpire = true;
            entries.push_back(entry);
        }

        void clear()
        {
            for (TimedParticleEntry& entry : entries)
            {
                if (entry.instance != nullptr)
                {
                    GameObjectAPI::removeGameObject(entry.instance);
                }
            }

            entries.clear();
        }
    };

    inline GameObject* instantiatePersistent(const AssetId& prefabId, const Vector3& position, const Vector3& rotation, GameObject* parent = nullptr)
    {
        if (!prefabId.isValid())
        {
            return nullptr;
        }

        GameObject* instance = GameObjectAPI::instantiatePrefab(prefabId, position, rotation, parent);
        if (instance == nullptr)
        {
            return nullptr;
        }

        disableSelfDestruct(instance);
        GameObjectAPI::setActive(instance, false);
        return instance;
    }

    inline GameObject* ensurePersistent(GameObject*& storage, const AssetId& prefabId, const Vector3& position, const Vector3& rotation, GameObject* parent = nullptr)
    {
        if (storage == nullptr)
        {
            storage = instantiatePersistent(prefabId, position, rotation, parent);
        }

        return storage;
    }

    inline void destroy(GameObject*& gameObject)
    {
        if (gameObject == nullptr)
        {
            return;
        }

        GameObjectAPI::removeGameObject(gameObject);
        gameObject = nullptr;
    }

    inline Transform* findChildRecursive(Transform* root, const char* name)
    {
        if (root == nullptr || name == nullptr || name[0] == '\0')
        {
            return nullptr;
        }

        Transform* direct = TransformAPI::findChildByName(root, name);
        if (direct != nullptr)
        {
            return direct;
        }

        const int childCount = TransformAPI::getChildCount(root);
        for (int i = 0; i < childCount; ++i)
        {
            Transform* found = findChildRecursive(TransformAPI::getChild(root, i), name);
            if (found != nullptr)
            {
                return found;
            }
        }

        return nullptr;
    }

    inline void syncToTransform(GameObject* instance, Transform* target)
    {
        if (instance == nullptr || target == nullptr)
        {
            return;
        }

        Transform* instanceTransform = GameObjectAPI::getTransform(instance);
        if (instanceTransform == nullptr)
        {
            return;
        }

        TransformAPI::setGlobalPosition(instanceTransform, TransformAPI::getGlobalPosition(target));
        TransformAPI::setGlobalRotationEuler(instanceTransform, TransformAPI::getGlobalEulerDegrees(target));
    }

    inline GameObject* spawnOneShot(const AssetId& prefabId, const Vector3& position, const Vector3& rotation = Vector3::Zero)
    {
        if (!prefabId.isValid())
        {
            return nullptr;
        }

        return GameObjectAPI::instantiatePrefab(prefabId, position, rotation, nullptr);
    }

    inline GameObject* spawnOneShotTimed(
        TimedParticleTracker& tracker,
        const AssetId& prefabId,
        const Vector3& position,
        const Vector3& rotation = Vector3::Zero,
        float lifetime = kDefaultOneShotLifetime
    )
    {
        GameObject* instance = spawnOneShot(prefabId, position, rotation);

        if (instance != nullptr)
        {
            tracker.scheduleDestroy(instance, lifetime);
        }

        return instance;
    }

    inline void activateTimed(
        TimedParticleTracker& tracker,
        GameObject* instance,
        float deactivateDelay = kDefaultPersistentDeactivateDelay
    )
    {
        if (instance == nullptr)
        {
            return;
        }

        activate(instance);
        tracker.scheduleDeactivate(instance, deactivateDelay);
    }
}
