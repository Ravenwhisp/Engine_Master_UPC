#pragma once

#include "Component.h"
#include <memory>
#include <string>

class Script;

class ScriptComponent : public Component
{
public:
    ScriptComponent(UID id, GameObject* owner);

    void setScript(std::unique_ptr<Script> script);
    Script* getScript() const;

    void setScriptName(const std::string& scriptName);
    const std::string& getScriptName() const;

    bool createScriptInstance();
    void destroyScriptInstance();

    void update() override;
    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

private:
    std::unique_ptr<Script> m_script;
    std::string m_scriptName;
    bool m_hasStarted = false;
};

