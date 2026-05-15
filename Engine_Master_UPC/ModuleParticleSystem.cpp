#include "Globals.h"
#include "ModuleParticleSystem.h"

#include "Application.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ModuleResources.h"
#include "ModuleTime.h"
#include "ParticleSystemComponent.h"


void ModuleParticleSystem::preRender()
{
	m_particleCommands.clear();

	for (auto& currentParticleSystemComponent : app->getModuleScene()->getParticleSystemComponents()) 
	{
		buildParticleCommands(currentParticleSystemComponent);
	}
}

void ModuleParticleSystem::update()
{
    for (auto& currentParticleSystemComponent : app->getModuleScene()->getParticleSystemComponents())
    {
        for (auto& emitter : currentParticleSystemComponent->getEmitterInstances())
        {
            emitter.updateModules();
        }
    }
}

ParticleSystem* ModuleParticleSystem::addSystem(Transform* parent)
{
	m_particleSystems.push_back(std::make_unique<ParticleSystem>());
	m_parents.push_back(parent);

	return m_particleSystems.back().get();
}

bool ModuleParticleSystem::removeSystem(ParticleSystem* system)
{
	for (unsigned int i = 0; i < m_particleSystems.size(); ++i)
	{
		if (m_particleSystems[i].get() == system)
		{
			m_particleSystems.erase(m_particleSystems.begin() + i);
			m_parents.erase(m_parents.begin() + i);
			return true;
		}
	}

    return false;
}

void ModuleParticleSystem::buildParticleCommands(ParticleSystemComponent* particleSystemComponent)
{
	if (!particleSystemComponent || !particleSystemComponent->isActive() || !particleSystemComponent->getOwner()->IsActiveInWindowHierarchy())
	{
		return;
	}

    if (particleSystemComponent->consumeLoadRequest()) // WILL NEED TO CHANGE TO CONSIDER SPECIFIC (AND MULTIPLE) EMITTERS IN THE COMPONENT
    {
        TextureAsset* asset = particleSystemComponent->getTextureAsset();
        MD5Hash assetId = particleSystemComponent->getTextureAssetId();

        if (!asset || assetId == INVALID_ASSET_ID)
        {
            particleSystemComponent->setTexture(nullptr);
        }
        else
        {
            auto textureIteration = m_particleTextures.find(assetId);
            if (textureIteration == m_particleTextures.end())
            {
                auto texture = app->getModuleResources()->createTextureSRGB(*asset, true);
                if (texture)
                {
                    Texture* raw = texture.get();
                    m_particleTextures.emplace(assetId, std::move(texture));
                    particleSystemComponent->setTexture(raw);
                }
                else
                {
                    particleSystemComponent->setTexture(nullptr);
                }
            }
            else
            {
                particleSystemComponent->setTexture(textureIteration->second.get());
            }
        }
    }

    // Command creation
    
    if (particleSystemComponent->getTexture() != nullptr)
    {

        for (auto& emitterInstance : particleSystemComponent->getEmitterInstances())
        {
            ParticleEmitterCommand command;
            command.texture = emitterInstance.getParticleEmitter()->getTexture();

            Particle* pool;
            std::vector<std::pair<float, unsigned int>>* aliveParticles;
            emitterInstance.getPoolAndAlives(pool, aliveParticles);

            for (auto& aliveParticle : *aliveParticles) 
            {
                ParticleCommand particleData;
                particleData.position = pool[aliveParticle.second].position;
                particleData.colorAndAlpha = pool[aliveParticle.second].colorAndAlpha;
                particleData.rotationZ = pool[aliveParticle.second].rotationZ;
                particleData.scale = pool[aliveParticle.second].scale;

                command.particles.push_back(particleData);
            }

            m_particleCommands.push_back(command);
        }
    }
    

}

float ModuleParticleSystem::deltaTime()
{
    if (app->getCurrentEngineState() == ENGINE_STATE::EDITOR)
    {
        return m_timeScale * app->getModuleTime()->unscaledDeltaTime();
    }

    return m_timeScale * app->getModuleTime()->deltaTime();
}
