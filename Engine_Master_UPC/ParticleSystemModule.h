#pragma once
#include "Module.h"

#include "ParticleSystem.h"

// THIS MODULE WON'T BE USED FOR NOW, BUT THE IDEA IS THAT IT HOLDS ALL PARTICLE SYSTEMS FOR EASY ACCESS

class ParticleSystemModule : public Module
{
public:

    //bool init()     override;
    //void preRender() override;
    //void render()   override;
    //bool cleanUp()  override;

    ParticleSystem* addSystem(Transform* parent);
    bool removeSystem(ParticleSystem* system);

private:

    std::vector<std::unique_ptr<ParticleSystem>> m_particleSystems;
    std::vector<Transform*> m_parents;
};

