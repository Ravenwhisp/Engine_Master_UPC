#include "Globals.h"
#include "ScriptComponent.h"
#include "Script.h"
#include "ScriptFactory.h"

ScriptComponent::ScriptComponent(UID id, GameObject* owner)
    : Component(id, ComponentType::SCRIPT, owner)
{
}

void ScriptComponent::setScript(std::unique_ptr<Script> script)
{
    m_script = std::move(script);
    m_hasStarted = false;
}

Script* ScriptComponent::getScript() const
{
    return m_script.get();
}

void ScriptComponent::setScriptName(const std::string& scriptName)
{
    m_scriptName = scriptName;
}

const std::string& ScriptComponent::getScriptName() const
{
    return m_scriptName;
}

bool ScriptComponent::createScriptInstance()
{
    if (m_scriptName.empty())
    {
        return false;
    }

    std::unique_ptr<Script> newScript = ScriptFactory::createScript(m_scriptName, getOwner());
    if (!newScript)
    {
        return false;
    }

    setScript(std::move(newScript));
    return true;
}


void ScriptComponent::update()
{
    if (!m_script)
    {
        return;
    }

    if (!m_hasStarted)
    {
        m_script->Start();
        m_hasStarted = true;
    }

    m_script->Update();
}

std::unique_ptr<Component> ScriptComponent::clone(GameObject* newOwner) const
{
    std::unique_ptr<ScriptComponent> clonedComponent = std::make_unique<ScriptComponent>(m_uuid, newOwner);
    clonedComponent->m_scriptName = m_scriptName;
    clonedComponent->m_hasStarted = false;
    return clonedComponent;
}