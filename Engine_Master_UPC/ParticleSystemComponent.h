#pragma once

#include "Component.h"
#include "TextureAsset.h"
#include "ParticleSystem.h"
#include "EmitterInstance.h"
#include "AssetReference.h"

class Texture;
class ModuleParticleSystem;

class ParticleSystemComponent : public Component
{
public:

    ParticleSystemComponent(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    void setTextureAssetId(AssetId& assetRef);
    AssetId& getTextureAssetId() { return m_textureAsset; }

    const Vector3& getPreviousPosition() const { return m_previousPosition; }
    float getDistance() const;

    std::vector<EmitterInstance>& getEmitterInstances() { return m_particlesState; }
    const std::vector<EmitterInstance>& getEmitterInstances() const { return m_particlesState; }

    void resetParticles();

    float getLocalTimeScale() const { return m_localTimeScale; }
    void setLocalTimeScale(float value) { m_localTimeScale = value; }
    float deltaTime() const;

    void drawUi() override;

    void update() override;

    void serialize(IArchive& archive) override;

    void debugDraw() override;

private:

    ModuleParticleSystem* m_moduleParticleSystem;

    AssetId m_textureAsset{};

    std::unique_ptr<ParticleSystem> m_particleSystem;
    std::vector<EmitterInstance> m_particlesState;

    unsigned int m_currentEditableEmitter = 0;

    Vector3 m_previousPosition; // maybe move this to the Transform?
    float m_localTimeScale = 1.f;
};

