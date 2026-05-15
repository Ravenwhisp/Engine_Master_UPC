#include "Globals.h"
#include "ParticleSystemComponent.h"

#include <imgui.h>

#include "Application.h"
#include "ModuleAssets.h"
#include "GameObject.h"
#include "ModuleParticleSystem.h"

#include "EmitterArea.h"
#include "EmitterColor.h"
#include "EmitterLifetime.h"
#include "EmitterSpawn.h"
#include "EmitterVelocity.h"

ParticleSystemComponent::ParticleSystemComponent(UID id, GameObject* owner) : Component(id, ComponentType::PARTICLE_SYSTEM, owner) 
{
    m_particleSystem = std::make_unique<ParticleSystem>();
    ParticleEmitter* firstEmitter = m_particleSystem->addEmitter(); // at least there should be one

    m_particlesState.push_back(EmitterInstance(firstEmitter, this));

    m_previousPosition = m_owner->GetTransform()->getPosition();
}

std::unique_ptr<Component> ParticleSystemComponent::clone(GameObject* newOwner) const
{
    std::unique_ptr<ParticleSystemComponent> cloned = std::make_unique<ParticleSystemComponent>(m_uuid, newOwner);

    cloned->m_textureAssetId = m_textureAssetId;
    cloned->m_gpuTexture = m_gpuTexture;
    cloned->m_textureAsset = m_textureAsset;
    cloned->m_loadRequested = false;
    cloned->setActive(this->isActive());

    // Emitters, Emitter instance cloning

    return cloned;
}

bool ParticleSystemComponent::consumeLoadRequest()
{
    const bool wasRequested = m_loadRequested;
    m_loadRequested = false;
    return wasRequested;
}

float ParticleSystemComponent::getDistance() const
{
    return Vector3::Distance(m_owner->GetTransform()->getPosition(), m_previousPosition);
}

void ParticleSystemComponent::drawUi()
{
    ImGui::Text("Particle system");

    ImGui::Button("Drop Here the Texture");

    ParticleEmitter& currentEmitter = m_particleSystem->getEmitters()[m_currentEditableEmitter];

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
        {
            const MD5Hash* data = static_cast<const MD5Hash*>(payload->Data);
            m_textureAssetId = *data;
            currentEmitter.setTexture(nullptr);
            m_gpuTexture = nullptr;
            m_textureAsset = app->getModuleAssets()->load<TextureAsset>(*data);
            DEBUG_LOG("Texture on particle system drop: assetId=%s", data->c_str());
            DEBUG_LOG("texture on particle system load result: %s", m_textureAsset ? "OK" : "NULL");
            if (m_textureAsset)
            {
                m_loadRequested = true;
                DEBUG_LOG("Texture on particle system load requested: %s", m_loadRequested ? "YES" : "NO");
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();

    ImGui::Text("Loaded: %s", (currentEmitter.getTexture() != nullptr) ? "YES" : "NO");

    if (!currentEmitter.getTexture()) return;

    // EMITTER INTERFACE

    // Speed controller //

    ImGui::Separator();

    float timeScale = app->getModuleParticleSystem()->getScale();
    if (timeScale <= 0.f) 
    {
        if (ImGui::Button("Play")) app->getModuleParticleSystem()->setScale(1.f);
    }
    else if (ImGui::Button("Stop")) app->getModuleParticleSystem()->setScale(0.f);

    if (ImGui::SliderFloat("Speed", &timeScale, 0.0f, 1.0f))
    {
        app->getModuleParticleSystem()->setScale(timeScale);
    }

    ImGui::Separator();

    // Modules parameters //

    bool parameterChanged = false;
    for (std::unique_ptr<ParticleModule>& module : currentEmitter.getModules()) 
    {
        parameterChanged = parameterChanged || module->drawUi();
        ImGui::Separator();
    }

    if (parameterChanged) 
    {
        for (auto& particleState : m_particlesState) particleState.reset();
    }
}

void ParticleSystemComponent::update()
{
    for (auto& particleState : m_particlesState)
    {
        particleState.updateModules();
    }

    m_previousPosition = m_owner->GetTransform()->getPosition(); // update previous position (maybe after threating particles?)
}

rapidjson::Value ParticleSystemComponent::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::SPRITE_RENDERER), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("TextureAssetId", rapidjson::Value(m_textureAssetId.c_str(), domTree.GetAllocator()), domTree.GetAllocator());

    return componentInfo;
}

bool ParticleSystemComponent::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("TextureAssetId"))
    {
        m_textureAssetId = componentInfo["TextureAssetId"].GetString();

        m_gpuTexture = nullptr;
        m_textureAsset = app->getModuleAssets()->load<TextureAsset>(m_textureAssetId);

        if (m_textureAsset)
        {
            m_loadRequested = true;
        }
    }

    return true;
}