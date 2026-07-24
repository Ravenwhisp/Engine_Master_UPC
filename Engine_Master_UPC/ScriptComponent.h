#pragma once

#include "Component.h"
#include <memory>
#include <string>
#include <rapidjson/document.h>

class Script;
class SceneReferenceResolver;
class IArchive;

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
    void resetStartState();
    void onGameStop();

    void update() override;
    void drawUi() override;
    void debugDraw() override;

    void serialize(IArchive& archive) override;
    void fixReferences(const SceneReferenceResolver& resolver) override;
    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    rapidjson::Value serializeScriptFieldsForReload(rapidjson::Document& domTree);
    void deserializeScriptFieldsForReload(const rapidjson::Value& fieldsJson);

private:
    void drawScriptFieldsUi(Script& script);
    void serializeScriptFields(Script& script, IArchive& archive);
    void cloneScriptFields(const Script& source, Script& target);

    std::unique_ptr<Script> m_script;
    std::string m_scriptName;
    bool m_hasStarted = false;
};

