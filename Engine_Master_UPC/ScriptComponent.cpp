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

void ScriptComponent::destroyScriptInstance()
{
    m_script.reset();
    m_hasStarted = false;
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

    //DEBUG_LOG("Here");
    m_script->Update();
}

void ScriptComponent::drawUi()
{
    char buffer[256];
    std::strncpy(buffer, m_scriptName.c_str(), sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    if (ImGui::InputText("Script Name", buffer, sizeof(buffer)))
    {
        m_scriptName = buffer;
    }

    if (ImGui::Button("Create Script Instance"))
    {
        bool created = createScriptInstance();
        if (created)
        {
            OutputDebugStringA("Script instance created correctly\n");
        }
        else
        {
            OutputDebugStringA("Failed to create script instance\n");
        }
    }

    ImGui::Text("Loaded: %s", m_script ? "Yes" : "No");
}

std::unique_ptr<Component> ScriptComponent::clone(GameObject* newOwner) const
{
    std::unique_ptr<ScriptComponent> clonedComponent = std::make_unique<ScriptComponent>(m_uuid, newOwner);
    clonedComponent->m_scriptName = m_scriptName;
    clonedComponent->m_hasStarted = false;
    return clonedComponent;
}

rapidjson::Value ScriptComponent::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", unsigned int(ComponentType::SCRIPT), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    return componentInfo;
}
