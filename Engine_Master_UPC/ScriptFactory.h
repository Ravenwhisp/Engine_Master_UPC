#pragma once

#include <memory>
#include <string>
#include <unordered_map>

class GameObject;
class Script;

using ScriptCreator = std::unique_ptr<Script>(*)(GameObject* owner);

class ScriptFactory
{
public:
    static void registerScript(const std::string& scriptName, ScriptCreator creator);
    static std::unique_ptr<Script> createScript(const std::string& scriptName, GameObject* owner);
    static bool isScriptRegistered(const std::string& scriptName);

private:
    static std::unordered_map<std::string, ScriptCreator> m_registry;
};