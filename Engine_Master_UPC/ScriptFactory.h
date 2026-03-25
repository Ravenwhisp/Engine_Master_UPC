#pragma once

#include "ScriptRegistry.h"
#include <vector>

class ScriptFactory
{
public:
    static void registerScript(const std::string& scriptName, ScriptCreator creator);
    static std::unique_ptr<Script> createScript(const std::string& scriptName, GameObject* owner);
    static bool isScriptRegistered(const std::string& scriptName);

private:
    static std::vector<ScriptRegistry> m_registry;
};