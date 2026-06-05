#include "Globals.h"
#include "ScriptComponent.h"
#include "JsonArchive.h"
#include "Script.h"
#include "GenericTypeFactory.h"
#include "SceneReferenceResolver.h"
#include "FieldUtils.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

ScriptComponent::ScriptComponent(UID id, GameObject* owner)
    : Component(id, ComponentType::SCRIPT, owner)
{
}

void ScriptComponent::setScript(std::unique_ptr<Script> script)
{
    m_script = std::move(script);
    resetStartState();
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

    std::unique_ptr<Script> newScript = ScriptFactory::create(m_scriptName, getOwner());
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
    resetStartState();
}

void ScriptComponent::resetStartState()
{
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

    if (ImGui::Button("Load Script"))
    {
        destroyScriptInstance();
        createScriptInstance();
    }

    ImGui::Text("Loaded: %s", m_script ? "Yes" : "No");

    if (!m_script)
    {
        return;
    }

    ImGui::SeparatorText("Script Variables");
    drawScriptFieldsUi(*m_script);
}

void ScriptComponent::debugDraw()
{
    if (m_script) 
    {
        m_script->drawGizmo();
    }
}

void ScriptComponent::drawScriptFieldsUi(Script& script)
{
    FieldUtils::drawUi(script, reinterpret_cast<char*>(&script));
}

void ScriptComponent::serializeScriptFields(Script& script, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree)
{
    FieldUtils::serialize(script, reinterpret_cast<const char*>(&script), outFieldsJson, domTree);
}

void ScriptComponent::serialize(IArchive& archive)
{
    Component::serialize(archive);

    archive.serialize(m_scriptName, "ScriptName");

    std::string fieldsJson;
    if (archive.mode() == ArchiveMode::Output && m_script)
    {
        rapidjson::Document doc;
        doc.SetObject();
        serializeScriptFields(*m_script, doc, doc);
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        fieldsJson = buffer.GetString();
    }

    archive.serialize(fieldsJson, "ScriptFields");

    if (archive.mode() == ArchiveMode::Input)
    {
        destroyScriptInstance();
        if (!m_scriptName.empty())
        {
            createScriptInstance();
        }

        if (m_script && !fieldsJson.empty())
        {
            rapidjson::Document doc;
            doc.Parse(fieldsJson.c_str());
            if (!doc.HasParseError() && doc.IsObject())
            {
                deserializeScriptFields(*m_script, doc);
                m_script->onAfterDeserialize();
            }
        }
    }
}

void ScriptComponent::deserializeScriptFields(Script& script, const rapidjson::Value& fieldsJson)
{
    FieldUtils::deserialize(script, reinterpret_cast<char*>(&script), fieldsJson);
}

void ScriptComponent::fixReferences(const SceneReferenceResolver& resolver)
{
    if (!m_script)
    {
        return;
    }

    FieldUtils::fixReferences(*m_script, reinterpret_cast<char*>(m_script.get()), resolver);

    m_script->onAfterReferencesFixed();
}

std::unique_ptr<Component> ScriptComponent::clone(GameObject* newOwner) const
{
    std::unique_ptr<ScriptComponent> clonedComponent = std::make_unique<ScriptComponent>(m_uuid, newOwner);
    clonedComponent->m_scriptName = m_scriptName;
    clonedComponent->m_hasStarted = false;
    clonedComponent->setActive(this->isActive());

    if (m_script && !m_scriptName.empty())
    {
        bool created = clonedComponent->createScriptInstance();
        assert(created);

        clonedComponent->cloneScriptFields(*m_script, *clonedComponent->m_script);
        clonedComponent->m_script->onAfterDeserialize();
    }

    return clonedComponent;
}

void ScriptComponent::cloneScriptFields(const Script& source, Script& target)
{
    FieldUtils::clone(source, reinterpret_cast<const char*>(&source), target, reinterpret_cast<char*>(&target));
}
