#pragma once
#include "Module.h"

#include "ParticleSystem.h"
#include "ParticleCommands.h"
#include "MD5Fwd.h"

class ParticleSystemComponent;

// THE IDEA IS THAT IT HOLDS ALL PARTICLE SYSTEMS FOR EASY ACCESS

class ModuleParticleSystem : public Module
{
public:

    //bool init()     override;
    void preRender() override;
    //void render()   override;
    //bool cleanUp()  override;

    ParticleSystem* addSystem(Transform* parent);
    bool removeSystem(ParticleSystem* system);

    void buildParticleCommands(ParticleSystemComponent* particleSystem);

    float deltaTime(); // required to have more control and show particles on Editor mode

private:

    std::vector<std::unique_ptr<ParticleSystem>> m_particleSystems;
    std::vector<Transform*> m_parents;

    std::vector<ParticleEmitterCommand> m_particleCommands; // we will probably want to directly get the shader parameters in the future
    std::unordered_map<MD5Hash, std::shared_ptr<Texture>> m_particleTextures;
};

