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
    void resetForPlay();

    void update() override;
    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentInfo) override;
    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

private:
    void drawScriptFieldsUi(Script& script);
    void serializeScriptFields(Script& script, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree);
    void deserializeScriptFields(Script& script, const rapidjson::Value& fieldsJson);
    void cloneScriptFields(const Script& source, Script& target);

    std::unique_ptr<Script> m_script;
    std::string m_scriptName;
    bool m_hasStarted = false;
};

