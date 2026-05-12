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

void ModuleParticleSystem::buildParticleCommands(ParticleSystemComponent* particleSystem)
{
	if (!particleSystem || !particleSystem->isActive() || !particleSystem->getOwner()->IsActiveInWindowHierarchy())
	{
		return;
	}

    if (particleSystem->consumeLoadRequest())
    {
        TextureAsset* asset = particleSystem->getTextureAsset();
        MD5Hash assetId = particleSystem->getTextureAssetId();

        if (!asset || assetId == INVALID_ASSET_ID)
        {
            particleSystem->setTexture(nullptr);
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
                    particleSystem->setTexture(raw);
                }
                else
                {
                    particleSystem->setTexture(nullptr);
                }
            }
            else
            {
                particleSystem->setTexture(textureIteration->second.get());
            }
        }
    }

    // Command creation (not done for now)
    /*
    if (particleSystem->getTexture() != nullptr)
    {
        UIImageCommand command;
        command.texture = uiImg->getTexture();
        command.rect = myRect;
        command.fillAmount = uiImg->getFillAmount();
        command.fillMethod = uiImg->getFillMethod();
        command.fillOrigin = uiImg->getFillOrigin();
        command.renderMode = renderMode;
        command.world = (renderMode == CanvasRenderMode::SCREEN_SPACE)
            ? Matrix::Identity
            : gameObject->GetTransform()->getGlobalMatrix();
        command.zTest = zTest;
        m_imageCommands.push_back(command);
    }
    */

}

float ModuleParticleSystem::deltaTime()
{
    if (app->getCurrentEngineState() == ENGINE_STATE::EDITOR)
    {
        return app->getModuleTime()->unscaledDeltaTime();
    }

    return app->getModuleTime()->deltaTime();
}
