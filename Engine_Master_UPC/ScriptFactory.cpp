#include "Globals.h"
#include "ScriptFactory.h"
#include "Script.h"

std::vector<ScriptRegistry> ScriptFactory::m_registry;

void ScriptFactory::registerScript(const std::string& scriptName, ScriptCreator creator)
{
    for (ScriptRegistry& script : m_registry)
    {
        if (script.name == scriptName)
        {
            script.creator = creator;
            return;
        }
    }

    m_registry.push_back({ scriptName, creator });
}

std::unique_ptr<Script> ScriptFactory::createScript(const std::string& scriptName, GameObject* owner)
{
    for (const ScriptRegistry& script : m_registry)
    {
        if (script.name == scriptName)
        {
            return script.creator(owner);
        }
    }

    return nullptr;
}

bool ScriptFactory::isScriptRegistered(const std::string& scriptName)
{
    for (const ScriptRegistry& script : m_registry)
    {
        if (script.name == scriptName)
        {
            return true;
        }
    }

    return false;
}