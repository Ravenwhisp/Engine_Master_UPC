#include "Globals.h"
#include "ScriptFactory.h"
#include "Script.h"

std::unordered_map<std::string, ScriptCreator> ScriptFactory::m_registry;

void ScriptFactory::registerScript(const std::string& scriptName, ScriptCreator creator)
{
    m_registry[scriptName] = creator;
}

std::unique_ptr<Script> ScriptFactory::createScript(const std::string& scriptName, GameObject* owner)
{
    auto it = m_registry.find(scriptName);
    if (it == m_registry.end())
    {
        return nullptr;
    }

    return it->second(owner);
}

bool ScriptFactory::isScriptRegistered(const std::string& scriptName)
{
    return m_registry.find(scriptName) != m_registry.end();
}