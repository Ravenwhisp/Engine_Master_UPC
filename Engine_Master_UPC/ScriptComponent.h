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

    template <class Archive>
    void save(Archive& ar) const
    {
        ar(cereal::base_class<Component>(this), m_scriptName);
    }

    template<class Archive>
    static void load_and_construct(
        Archive& ar,
        cereal::construct<ScriptComponent>& construct)
    {
        UID           id;
        ComponentType type;
        bool          active;
        ar(id, type, active);

        construct(id, nullptr);
        construct->setActive(active);

        ar(construct->m_scriptName);
    }


private:
    void drawScriptFieldsUi(Script& script);
    void serializeScriptFields(Script& script, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree);
    void deserializeScriptFields(Script& script, const rapidjson::Value& fieldsJson);
    void cloneScriptFields(const Script& source, Script& target);

   

    void saveBinaryScriptFields(const Script& script, cereal::BinaryOutputArchive& ar) const;
    void loadBinaryScriptFields(Script& script, cereal::BinaryInputArchive& ar);

    std::unique_ptr<Script> m_script;
    std::string m_scriptName;
    bool m_hasStarted = false;
};

