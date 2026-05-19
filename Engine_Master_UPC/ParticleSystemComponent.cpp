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

    m_moduleParticleSystem = app->getModuleParticleSystem();
}

std::unique_ptr<Component> ParticleSystemComponent::clone(GameObject* newOwner) const
{
    std::unique_ptr<ParticleSystemComponent> cloned = std::make_unique<ParticleSystemComponent>(m_uuid, newOwner);

    cloned->m_textureAsset = m_textureAsset;
    cloned->m_currentEditableEmitter = m_currentEditableEmitter;
    cloned->setActive(this->isActive());

    // Emitters, Emitter instance cloning

    cloned->m_particleSystem.reset(new ParticleSystem(*m_particleSystem));

    auto& clonedEmitters = cloned->m_particleSystem->getEmitters();
    cloned->m_particlesState.clear();
    cloned->m_particlesState.reserve(clonedEmitters.size());
    for (auto& clonedEmitter : clonedEmitters) 
    {
        cloned->m_particlesState.push_back(EmitterInstance(&clonedEmitter, cloned.get()));
    }

    return cloned;
}

void ParticleSystemComponent::setTextureAssetReference(AssetReference& assetRef)
{
    m_textureAsset = assetRef;
}

float ParticleSystemComponent::getDistance() const
{
    return Vector3::Distance(m_owner->GetTransform()->getPosition(), m_previousPosition);
}

void inline ParticleSystemComponent::resetParticles()
{
    for (auto& particleState : m_particlesState) particleState.reset();

    m_previousPosition = m_owner->GetTransform()->getPosition(); // just in case
}

float ParticleSystemComponent::deltaTime() const 
{
    return m_localTimeScale * m_moduleParticleSystem->deltaTime();
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
            UID* ref = static_cast<UID*>(payload->Data);
            AssetReference* assetRef = app->getModuleAssets()->findReference(*ref);
            if (assetRef)
            {
                m_textureAsset = *assetRef;
                DEBUG_LOG("Texture on particle system drop: assetId=%s", assetRef->m_libId.c_str());
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();

    ImGui::Text("Loaded: %s", m_textureAsset.isValid() ? "YES" : "NO");

    if (!m_textureAsset.isValid()) return;

    // EMITTER INTERFACE

    // Speed controller //

    ImGui::Separator();

    if (m_localTimeScale <= 0.f) 
    {
        if (ImGui::Button("Play")) m_localTimeScale = 1.f;
    }
    else if (ImGui::Button("Stop")) m_localTimeScale = 0.f;

    
    ImGui::SameLine();
    if (ImGui::Button("Reset")) resetParticles();
    // Restart button (temporary implementation, until we can have more than one emitter per component)
    ImGui::SameLine();
    if (ImGui::Button("Restart all")) m_moduleParticleSystem->resetAllParticles();


    ImGui::SliderFloat("Speed", &m_localTimeScale, 0.0f, 1.0f);
   
    ImGui::Separator();

    // Modules parameters //

    bool parameterChanged = false;
    for (std::unique_ptr<ParticleModule>& module : currentEmitter.getModules()) 
    {
        parameterChanged |= module->drawUi();
        ImGui::Separator();
    }

    if (parameterChanged) resetParticles();
}

void ParticleSystemComponent::update()
{
    if (m_textureAsset.isValid()) 
    {
            for (auto& particleState : m_particlesState)
        {
            particleState.updateModules();
        }
    }

    m_previousPosition = m_owner->GetTransform()->getPosition(); // update previous position
}

rapidjson::Value ParticleSystemComponent::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::PARTICLE_SYSTEM), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    // Should go on the Emitter class? (or at least have an array of texture assets, one per emitter, in the future)
    componentInfo.AddMember("TextureAssetId", m_textureAsset.getJson(domTree.GetAllocator()), domTree.GetAllocator());


    rapidjson::Value emitterData(rapidjson::kArrayType);
    for (auto& emitter : m_particleSystem->getEmitters())
    {
        emitterData.PushBack(emitter.getJSON(domTree), domTree.GetAllocator());
    }

    componentInfo.AddMember("ParticleEmitters", emitterData, domTree.GetAllocator());

    return componentInfo;
}

bool ParticleSystemComponent::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("TextureAssetId"))
    {
        m_textureAsset.deserializeJson(componentInfo["TextureAssetId"]);
    }

    // Emitters and instances set up //
    if (!componentInfo.HasMember("ParticleEmitters")) return false;

    const rapidjson::Value& emittersInfo = componentInfo["ParticleEmitters"];

    m_particleSystem.reset(new ParticleSystem(emittersInfo.Size()));

    auto& emitters = m_particleSystem->getEmitters();
    m_particlesState.clear();
    m_particlesState.reserve(emitters.size());

    for (unsigned int i = 0; i < emitters.size(); ++i)
    {
        emitters[i].deserializeJSON(emittersInfo[i]);

        m_particlesState.push_back(EmitterInstance(&emitters[i], this));
    }

    return true;
}

void ParticleSystemComponent::debugDraw()
{
    ParticleEmitter& currentEmitter = m_particleSystem->getEmitters()[m_currentEditableEmitter];
    Transform* parentTransform = m_owner->GetTransform();

    for (std::unique_ptr<ParticleModule>& module : currentEmitter.getModules())
    {
        module->debugDraw(parentTransform);
    }
}
