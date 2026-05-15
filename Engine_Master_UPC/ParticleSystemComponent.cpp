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
            DEBUG_LOG("Texture on particle system drop: assetId=%s", data->c_str());
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();

    ImGui::Text("Loaded: %s", (m_textureAssetId != INVALID_ASSET_ID) ? "YES" : "NO");

    if (m_textureAssetId == INVALID_ASSET_ID) return;

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
        parameterChanged |= module->drawUi();
        ImGui::Separator();
    }

    if (parameterChanged) 
    {
        for (auto& particleState : m_particlesState) particleState.reset();
    }
}

void ParticleSystemComponent::update()
{
    /* RIGHT NOW EXECUTON HAPPENS IN PARTICLESYSTEMODULE
    for (auto& particleState : m_particlesState)
    {
        particleState.updateModules();
    }
    */


    

    m_previousPosition = m_owner->GetTransform()->getPosition(); // update previous position (maybe after threating particles?)
}

rapidjson::Value ParticleSystemComponent::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::PARTICLE_SYSTEM), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    // Should go on the Emitter class? (or at least have an array of texture assets, one per emitter, in the future)
    componentInfo.AddMember("TextureAssetId", rapidjson::Value(m_textureAssetId.c_str(), domTree.GetAllocator()), domTree.GetAllocator());


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
        m_textureAssetId = componentInfo["TextureAssetId"].GetString();
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