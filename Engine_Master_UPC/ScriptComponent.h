#pragma once

#include "Component.h"
#include <memory>
#include <string>
#include "Script.h"

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

    void update() override;
    void drawUi() override;
    void debugDraw() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentInfo) override;
    void fixReferences(const SceneReferenceResolver& resolver) override;
    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

private:
    void drawScriptFieldsUi(Script& script);
    void serializeScriptFields(Script& script, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree);
    void deserializeScriptFields(Script& script, const rapidjson::Value& fieldsJson);
    void cloneScriptFields(const Script& source, Script& target);

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(cereal::base_class<Component>(this), m_scriptName);

        if constexpr (Archive::is_saving::value)
        {
            bool hasScript = m_script != nullptr;
            ar(hasScript);
            if (hasScript)
            {
                saveBinaryScriptFields(*m_script, ar);
            }
        }
        else
        {
            bool hasScript = false;
            ar(hasScript);

            destroyScriptInstance();
            if (!m_scriptName.empty())
            {
                createScriptInstance();
            }

            if (hasScript && m_script)
            {
                loadBinaryScriptFields(*m_script, ar);
                m_script->onAfterDeserialize();
            }
        }
    }


    void saveBinaryScriptFields(const Script& script, cereal::BinaryOutputArchive& ar) const;
    void loadBinaryScriptFields(Script& script, cereal::BinaryInputArchive& ar);

    std::unique_ptr<Script> m_script;
    std::string m_scriptName;
    bool m_hasStarted = false;
};

