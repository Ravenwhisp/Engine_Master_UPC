#pragma once

#include "ParticleModule.h"
#include "Transform.h"
#include "Texture.h"
#include "EmitterRender.h"
#include <vector>
#include <utility>

class EmitterLifetime;
class EmitterAnimation;

class ParticleEmitter
{
public:

	ParticleEmitter();
	ParticleEmitter(const ParticleEmitter& particleEmitter);

	ParticleModule* getModule(ParticleModuleType type);
	std::vector<std::unique_ptr<ParticleModule>>& getModules() { return m_particleModules; }

	void setTexture(Texture* texture) { m_texture = texture; }
	Texture* getTexture() { return m_texture; }

	EmitterLifetime* getLifetimeModule() { return m_lifetimeModule; }
	EmitterAnimation* getAnimationModule() { return m_animationModule;  }
	EmitterRender* getRenderModule() { return m_renderModule; }

	rapidjson::Value getJSON(rapidjson::Document& domTree);
	bool deserializeJSON(const rapidjson::Value& emitterInfo);

private:

	Texture* m_texture = nullptr;
	
	std::vector<std::unique_ptr<ParticleModule>> m_particleModules;
	EmitterLifetime* m_lifetimeModule;
	EmitterAnimation* m_animationModule;
	EmitterRender* m_renderModule;
};

