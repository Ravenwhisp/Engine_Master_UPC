#pragma once
#include "Module.h"

#include "ParticleEmitter.h"

class ParticleSystemModule : public Module
{
public:

    //bool init()     override;
    //void preRender() override;
    //void render()   override;
    //bool cleanUp()  override;

    ParticleEmitter* addEmitter(Transform* parent);
    bool removeEmitter(ParticleEmitter* emitter);

private:

    std::vector<std::unique_ptr<ParticleEmitter>> m_emitters;
};

