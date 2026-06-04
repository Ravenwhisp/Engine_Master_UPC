#include "Globals.h"
#include "ScriptComponent.h"
#include "JsonArchive.h"
#include "Script.h"
#include "ScriptFactory.h"
#include "SceneReferenceResolver.h"

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

    std::unique_ptr<Script> newScript = ScriptFactory::createScript(m_scriptName, getOwner());
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
    ScriptFieldList fieldList = script.getExposedFields();
    char* base = reinterpret_cast<char*>(&script);

    bool currentGroupOpen = true;

    for (const ScriptFieldInfo& field : fieldList.fields)
    {
        if (field.type == ScriptFieldType::GroupCollapseBegin)
        {
            ImGui::Spacing();

            currentGroupOpen = ImGui::CollapsingHeader(field.name, ImGuiTreeNodeFlags_DefaultOpen);
            continue;
        }

        if (field.type == ScriptFieldType::GroupCollapseEnd)
        {
            currentGroupOpen = true;
            continue;
        }

        if (!currentGroupOpen)
        {
            continue;
        }

        void* data = base + field.offset;

        assert(field.handler != nullptr);
        field.handler->drawUi(field, data, script, *this);
    }
}

void ScriptComponent::serializeScriptFields(Script& script, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree)
{
    ScriptFieldList fieldList = script.getExposedFields();
    char* base = reinterpret_cast<char*>(&script);

    for (const ScriptFieldInfo& field : fieldList.fields)
    {
        if (!field.isDataField())
        {
            continue;
        }

        const void* data = base + field.offset;

        assert(field.handler != nullptr);
        field.handler->serialize(field, data, outFieldsJson, domTree);
    }
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
    ScriptFieldList fieldList = script.getExposedFields();
    char* base = reinterpret_cast<char*>(&script);

    for (const ScriptFieldInfo& field : fieldList.fields)
    {
        if (!field.isDataField())
        {
            continue;
        }

        if (!fieldsJson.HasMember(field.name))
        {
            continue;
        }

        void* data = base + field.offset;
        const rapidjson::Value& valueJson = fieldsJson[field.name];

        assert(field.handler != nullptr);
        field.handler->deserialize(field, data, valueJson);
    }
}

void ScriptComponent::fixReferences(const SceneReferenceResolver& resolver)
{
    if (!m_script)
    {
        return;
    }

    ScriptFieldList fieldList = m_script->getExposedFields();
    char* base = reinterpret_cast<char*>(m_script.get());

    for (const ScriptFieldInfo& field : fieldList.fields)
    {
        if (!field.isDataField())
        {
            continue;
        }

        void* data = base + field.offset;

        assert(field.handler != nullptr);
        field.handler->fixReferences(field, data, resolver);
    }

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
    ScriptFieldList sourceFields = source.getExposedFields();
    ScriptFieldList targetFields = target.getExposedFields();

    const size_t count = std::min(sourceFields.fields.size(), targetFields.fields.size());

    const char* sourceBase = reinterpret_cast<const char*>(&source);
    char* targetBase = reinterpret_cast<char*>(&target);

    for (size_t i = 0; i < count; ++i)
    {
        const ScriptFieldInfo& sourceField = sourceFields.fields[i];
        const ScriptFieldInfo& targetField = targetFields.fields[i];

        if (!sourceField.isDataField() || !targetField.isDataField())
        {
            continue;
        }

        assert(sourceField.handler == targetField.handler);

        const void* sourceData = sourceBase + sourceField.offset;
        void* targetData = targetBase + targetField.offset;

        assert(sourceField.handler != nullptr);
        assert(targetField.handler != nullptr);

        sourceField.handler->clone(sourceField, sourceData, targetData);
    }
}