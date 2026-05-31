#include "Globals.h"
#include "ModuleParticleSystem.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ModuleResources.h"
#include "ModuleTime.h"
#include "ParticleSystemComponent.h"


void ModuleParticleSystem::resetAllParticles()
{
    for (auto& currentParticleSystemComponent : app->getModuleScene()->getParticleSystemComponents())
    {
        currentParticleSystemComponent->resetParticles();
        currentParticleSystemComponent->setLocalTimeScale(1.f); // FOR NOW, SO THAT TRANSITION TO PLAY MODE WORKS WELL
    }

    m_timeScale = 1.f; // ALSO FOR NOW, FOR THE SAME REASON
}

Texture* ModuleParticleSystem::resolveTexture(AssetReference& textureRef)
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
    m_slots[MAX_PARTICLES - 1] = 0;

    for (unsigned int i = 0; i < MAX_PARTICLES - 1; ++i)
    {
        m_slots[i] = i + 1;
    }

    m_firstFree = 0;
}

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
    if (app->getCurrentEngineState() != ENGINE_STATE::EDITOR) return;

    for (auto& currentParticleSystemComponent : app->getModuleScene()->getParticleSystemComponents())
    {
        currentParticleSystemComponent->update();
    }
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

int ModuleParticleSystem::requestPoolSlot()
{

    if (m_firstFree == m_slots[m_firstFree]) return -1; // because the slot points to itself, which indicates used

    int slot = m_firstFree;

    m_firstFree = m_slots[slot]; // update first free, to point to the next one

    m_slots[slot] = slot; // mark as used

    return slot;
}

void ModuleParticleSystem::freePoolSlot(unsigned int index) 
{
    m_slots[index] = m_firstFree; // Set next free (because we are adding the index slot as the new first)

    m_firstFree = index; // Update first
}



void ModuleParticleSystem::buildParticleCommands(ParticleSystemComponent* particleSystemComponent)
{
    if (!particleSystemComponent || !particleSystemComponent->isActive() || !particleSystemComponent->getOwner()->IsActiveInWindowHierarchy())
    {
        return;
    }

    AssetReference& textureRef = particleSystemComponent->getTextureAssetReference();
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

        ParticleEmitterCommand command;
		command.texture = texture;
		command.particles.reserve(aliveParticles.size());

		for (const auto& aliveParticle : aliveParticles)
		{
			ParticleCommand particleData;
			particleData.position = m_pool[aliveParticle.second].position;
			particleData.colorAndAlpha = m_pool[aliveParticle.second].colorAndAlpha;
			particleData.rotationZ = m_pool[aliveParticle.second].rotationZ;
			particleData.scale = m_pool[aliveParticle.second].scale;

			command.particles.push_back(particleData);
		}

		m_particleCommands.push_back(std::move(command));
    }
}

float ModuleParticleSystem::deltaTime() const
{
    return m_timeScale * app->getModuleTime()->deltaTime();
}
