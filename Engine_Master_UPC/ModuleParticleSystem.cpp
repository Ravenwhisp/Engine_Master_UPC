#include "Globals.h"
#include "ModuleParticleSystem.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ModuleResources.h"
#include "ModuleTime.h"

#include "ParticleSystemComponent.h"
#include "EmitterAnimation.h"

#include "ParticleEmitter.h"
#include "EmitterRender.h"


void ModuleParticleSystem::resetAllParticles()
{
    for (auto& currentParticleSystemComponent : app->getModuleScene()->getParticleSystemComponents())
    {
        currentParticleSystemComponent->resetParticles();
        currentParticleSystemComponent->setLocalTimeScale(1.f); // FOR NOW, SO THAT TRANSITION TO PLAY MODE WORKS WELL
    }

    resetFirstUsedSlot();

    m_timeScale = 1.f; // ALSO FOR NOW, FOR THE SAME REASON
}

Texture* ModuleParticleSystem::resolveTexture(AssetId& textureRef)
{
    if (!textureRef.isValid())
    {
        return nullptr;
    }

    const MD5Hash& libId = textureRef.m_libId;
    auto it = m_particleTextures.find(libId);
    if (it != m_particleTextures.end())
    {
        return it->second.get();
    }

    std::shared_ptr<TextureAsset> asset = app->getModuleAssets()->load<TextureAsset>(textureRef);
    if (!asset)
    {
        DEBUG_WARN("[ModuleParticleSystem] Missing TextureAsset: %s", libId.c_str());
        return nullptr;
    }

    auto texture = app->getModuleResources()->createTexture(*asset, true);
    if (!texture)
    {
        DEBUG_WARN("[ModuleParticleSystem] Texture creation failed: %s", libId.c_str());
        return nullptr;
    }

    Texture* raw = texture.get();
    m_particleTextures.emplace(libId, std::move(texture));
    return raw;
}


bool ModuleParticleSystem::init()
{
    initSlotManagement();

    return true;
}

void ModuleParticleSystem::initSlotManagement()
{
    m_freeSlots.reserve(MAX_PARTICLES);

    for (int i = MAX_PARTICLES-1; i >= 0; --i)
    {
        m_freeSlots.push_back(i);
    }

   m_firstUsed = 0; // used here means nothing, but it will be true once we reserve the first slot
}

void ModuleParticleSystem::preRender()
{
	m_particleCommands.clear();

	for (auto& currentParticleSystemComponent : app->getModuleScene()->getParticleSystemComponents()) 
	{
		buildParticleCommands(currentParticleSystemComponent);
	}

    // sort m_particleCommands per layer value (so that overlapped emitters with higher layer are rendered on top)
    std::sort(m_particleCommands.begin(), m_particleCommands.end(), [](const ParticleEmitterCommand& a, const ParticleEmitterCommand& b)
    {
        return a.layer < b.layer;
    });
}

void ModuleParticleSystem::update()
{
    if (app->getCurrentEngineState() == ENGINE_STATE::PAUSED) return;

    /*
    for (auto& currentParticleSystemComponent : app->getModuleScene()->getParticleSystemComponents())
    {
        currentParticleSystemComponent->update();
    }
    */

    for (auto& currentParticleSystemComponent : app->getModuleScene()->getParticleSystemComponents())
    {
        currentParticleSystemComponent->updateSpawn();
    }

    for (auto& currentParticleSystemComponent : app->getModuleScene()->getParticleSystemComponents())
    {
        currentParticleSystemComponent->updateTheRest();
    }
}

bool ModuleParticleSystem::cleanUp()
{
    m_particleCommands.clear();

    m_particleTextures.clear(); // needed so that textures are freed at the right time everywhere else

    return true;
}

/*
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
*/

int ModuleParticleSystem::requestPoolSlot(EmitterInstance* newOwner)
{

    if (m_freeSlots.empty()) { // => we have to reuse an used one

        int slot = m_firstUsed;
        m_firstUsed = (++m_firstUsed) % MAX_PARTICLES; // move to next in the pool (approximated)

        if (!m_pool[slot].owner->removeNewParticle(slot)) { // erase in case it was a new one for the previous owner (makes sense since we are doing the spawn first for all the emitters)

            m_pool[slot].movedFromAlives = true; // means previous owner had it for at least one frame, so we need to tell it to remove it from alives
        } 
        m_pool[slot].owner = newOwner;
        return slot;
    }

    int slot = m_freeSlots.back();
    m_freeSlots.pop_back();

    m_pool[slot].owner = newOwner;
    return slot;
}

void ModuleParticleSystem::freePoolSlot(unsigned int index) 
{
    m_freeSlots.push_back(index);

    if (index == m_firstUsed) m_firstUsed = (++m_firstUsed) % MAX_PARTICLES; // move to next in the pool (approximated)
}

void ModuleParticleSystem::resetFirstUsedSlot()
{
    m_firstUsed = m_freeSlots.back(); // AT LEAST there should be one
}

void ModuleParticleSystem::buildParticleCommands(ParticleSystemComponent* particleSystemComponent)
{
    if (!particleSystemComponent || !particleSystemComponent->isActive() || !particleSystemComponent->getOwner()->IsActiveInWindowHierarchy())
    {
        return;
    }

    AssetId& textureRef = particleSystemComponent->getTextureAssetId();
    Texture* texture = resolveTexture(textureRef);
    if (!texture)
    {
        return;
    }

    for (auto& emitterInstance : particleSystemComponent->getEmitterInstances())
    {
        std::vector<std::pair<float, unsigned int>>& aliveParticles = emitterInstance.getAliveParticles();

        if (aliveParticles.empty())
        {
            continue;
        }

        EmitterAnimation* animationConfig = emitterInstance.getParticleEmitter()->getAnimationModule();
        EmitterRender* renderConfig = emitterInstance.getParticleEmitter()->getRenderModule();

        ParticleEmitterCommand command;
		command.texture = texture;
        command.layer = renderConfig->getLayer();
        command.uvScale = animationConfig->getUVScale();

		command.particles.reserve(aliveParticles.size());
        command.renderMode = renderConfig->getRenderMode();
        command.particles.reserve(aliveParticles.size());

		for (const auto& aliveParticle : aliveParticles)
		{
			ParticleCommand particleData;
			particleData.position = m_pool[aliveParticle.second].position;
			particleData.colorAndAlpha = m_pool[aliveParticle.second].colorAndAlpha;
			particleData.rotationZ = m_pool[aliveParticle.second].rotationZ;
			particleData.scale = m_pool[aliveParticle.second].scale;
            
            particleData.sheetOffset = animationConfig->getUVOffset(aliveParticle.second);

			command.particles.push_back(particleData);
		}

		m_particleCommands.push_back(std::move(command));
    }
}

float ModuleParticleSystem::deltaTime() const
{
    return m_timeScale * app->getModuleTime()->deltaTime();
}
