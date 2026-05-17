#pragma once
#include "Module.h"

#include "ParticleSystem.h"
#include "ParticleCommands.h"
#include "AssetReference.h"

class ParticleSystemComponent;

// IN THE FUTURE, THE IDEA IS THAT IT HOLDS ALL PARTICLE SYSTEMS FOR EASY ACESS

class ModuleParticleSystem : public Module
{
public:

    //bool init()     override;
    void preRender()  override;
    void update()     override;
    //void render()   override;
    //bool cleanUp()  override;

    ParticleSystem* addSystem(Transform* parent);
    bool removeSystem(ParticleSystem* system);

    void buildParticleCommands(ParticleSystemComponent* particleSystemComponent);

    const std::vector<ParticleEmitterCommand>& getParticleCommands() const { return m_particleCommands; }

    float deltaTime() const; // required to have more control on Editor mode

    void setTimeScale(float value) { m_timeScale = value; };
    float getTimeScale() const { return m_timeScale; };

    void resetAllParticles();

private:

    Texture* resolveTexture(AssetReference& textureRef);

    std::vector<std::unique_ptr<ParticleSystem>> m_particleSystems;
    std::vector<Transform*> m_parents;

    std::vector<ParticleEmitterCommand> m_particleCommands; // we will probably want to directly get the shader parameters in the future
    std::unordered_map<MD5Hash, std::shared_ptr<Texture>> m_particleTextures;

    float m_timeScale = 1.f; // for particle speed control
};

