#include "Globals.h"
#include "ScriptComponent.h"
#include "Script.h"
#include "ScriptFactory.h"
#include "ScriptComponentRef.h"
#include "SceneReferenceResolver.h"

#include "Application.h"
#include "ModuleScene.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"

#include "ModuleAssets.h"
#include <Metadata.h>

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
    drawScriptFieldsUi(*m_script);
}

void ScriptComponent::drawScriptFieldsUi(Script& script)
{
    ScriptFieldList fieldList = script.getExposedFields();
    char* base = reinterpret_cast<char*>(&script);

    for (size_t i = 0; i < fieldList.count; ++i)
    {
        const ScriptFieldInfo& field = fieldList.fields[i];
        void* data = base + field.offset;
        bool changed = false;

        switch (field.type)
        {
        case ScriptFieldType::Float:
        {
            float* value = reinterpret_cast<float*>(data);
            changed = ImGui::DragFloat(field.name, value, field.floatInfo.dragSpeed, field.floatInfo.min, field.floatInfo.max);
            break;
        }

        case ScriptFieldType::Int:
        {
            int* value = reinterpret_cast<int*>(data);
            changed = ImGui::DragInt(field.name, value);
            break;
        }

        case ScriptFieldType::Bool:
        {
            bool* value = reinterpret_cast<bool*>(data);
            changed = ImGui::Checkbox(field.name, value);
            break;
        }

        case ScriptFieldType::Vec3:
        {
            Vector3* value = reinterpret_cast<Vector3*>(data);
            changed = ImGui::DragFloat3(field.name, &value->x, 0.1f);
            break;
        }

        case ScriptFieldType::EnumInt:
        {
            int* value = reinterpret_cast<int*>(data);

            const char* preview = "";
            if (*value >= 0 && *value < field.enumInfo.count)
            {
                preview = field.enumInfo.names[*value];
            }

            if (ImGui::BeginCombo(field.name, preview))
            {
                for (int enumIndex = 0; enumIndex < field.enumInfo.count; ++enumIndex)
                {
                    bool selected = (*value == enumIndex);
                    if (ImGui::Selectable(field.enumInfo.names[enumIndex], selected))
                    {
                        *value = enumIndex;
                        changed = true;
                    }

                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            break;
        }

        case ScriptFieldType::ComponentRef:
        {
            ScriptComponentRef<Component>* componentReference = reinterpret_cast<ScriptComponentRef<Component>*>(data);

            Component* component = componentReference->component;

            if (component)
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", component->getOwner()->GetName().c_str() );
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "None");
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAME_OBJECT"))
                {
                    GameObject* droppedObject = *(GameObject**)payload->Data;
                    GameObject* sceneObject = app->getModuleScene()->getScene()->findGameObjectByUID(droppedObject->GetID());

                    if (sceneObject)
                    {
                        Component* candidate = nullptr;

                        if (field.componentRefInfo.componentType == ComponentType::TRANSFORM)
                        {
                            candidate = sceneObject->GetTransform();
                        }
                        else
                        {
                            candidate = sceneObject->GetComponent(field.componentRefInfo.componentType);
                        }

                        if (candidate)
                        {
                            componentReference->uid = candidate->getID();
                            componentReference->component = candidate;
                            changed = true;
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::SameLine();

            std::string clearLabel = std::string("Clear###") + field.name;
            if (ImGui::Button(clearLabel.c_str()))
            {
                componentReference->uid = 0;
                componentReference->component = nullptr;
                changed = true;
            }

            break;
        }

        case ScriptFieldType::String:
        {
            std::string* value = reinterpret_cast<std::string*>(data);

            char buffer[256];
            std::strncpy(buffer, value->c_str(), sizeof(buffer));
            buffer[sizeof(buffer) - 1] = '\0';

            if (ImGui::InputText(field.name, buffer, sizeof(buffer)))
            {
                *value = buffer;
                changed = true;
            }
            break;
        }
        }

        if (changed)
        {
            script.onFieldEdited(field);
        }
    }
}

rapidjson::Value ScriptComponent::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", unsigned int(ComponentType::SCRIPT), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("ScriptName", rapidjson::Value(m_scriptName.c_str(), domTree.GetAllocator()), domTree.GetAllocator());

    rapidjson::Value fieldsJson(rapidjson::kObjectType);

    if (m_script)
    {
        serializeScriptFields(*m_script, fieldsJson, domTree);
    }

    componentInfo.AddMember("ScriptFields", fieldsJson, domTree.GetAllocator());

    return componentInfo;
}

void ScriptComponent::serializeScriptFields(Script& script, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree)
{
    ScriptFieldList fieldList = script.getExposedFields();
    char* base = reinterpret_cast<char*>(&script);

    for (size_t i = 0; i < fieldList.count; ++i)
    {
        const ScriptFieldInfo& field = fieldList.fields[i];
        void* data = base + field.offset;

        rapidjson::Value key(field.name, domTree.GetAllocator());

        switch (field.type)
        {
        case ScriptFieldType::Float:
            outFieldsJson.AddMember(key, *reinterpret_cast<float*>(data), domTree.GetAllocator());
            break;

        case ScriptFieldType::Int:
            outFieldsJson.AddMember(key, *reinterpret_cast<int*>(data), domTree.GetAllocator());
            break;

        case ScriptFieldType::Bool:
            outFieldsJson.AddMember(key, *reinterpret_cast<bool*>(data), domTree.GetAllocator());
            break;

        case ScriptFieldType::EnumInt:
            outFieldsJson.AddMember(key, *reinterpret_cast<int*>(data), domTree.GetAllocator());
            break;

        case ScriptFieldType::Vec3:
        {
            Vector3* value = reinterpret_cast<Vector3*>(data);

            rapidjson::Value array(rapidjson::kArrayType);
            array.PushBack(value->x, domTree.GetAllocator());
            array.PushBack(value->y, domTree.GetAllocator());
            array.PushBack(value->z, domTree.GetAllocator());

            outFieldsJson.AddMember(key, array, domTree.GetAllocator());
            break;
        }

        case ScriptFieldType::ComponentRef:
        {
            ScriptComponentRef<Component>* componentReference = reinterpret_cast<ScriptComponentRef<Component>*>(data);

            outFieldsJson.AddMember(key, static_cast<uint64_t>(componentReference->uid), domTree.GetAllocator());
            break;
        }

        case ScriptFieldType::String:
        {
            std::string* value = reinterpret_cast<std::string*>(data);
            outFieldsJson.AddMember(key, rapidjson::Value(value->c_str(), domTree.GetAllocator()), domTree.GetAllocator());
            break;
        }
        }
    }
}

bool ScriptComponent::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("ScriptName"))
    {
        m_scriptName = componentInfo["ScriptName"].GetString();
    }

    destroyScriptInstance();

    if (!m_scriptName.empty())
    {
        createScriptInstance();
    }

    if (m_script && componentInfo.HasMember("ScriptFields"))
    {
        deserializeScriptFields(*m_script, componentInfo["ScriptFields"]);
        m_script->onAfterDeserialize();
    }

    return true;
}

void ScriptComponent::deserializeScriptFields(Script& script, const rapidjson::Value& fieldsJson)
{
    ScriptFieldList fieldList = script.getExposedFields();
    char* base = reinterpret_cast<char*>(&script);

    for (size_t i = 0; i < fieldList.count; ++i)
    {
        const ScriptFieldInfo& field = fieldList.fields[i];

        if (!fieldsJson.HasMember(field.name))
        {
            continue;
        }

        void* data = base + field.offset;
        const rapidjson::Value& valueJson = fieldsJson[field.name];

        switch (field.type)
        {
        case ScriptFieldType::Float:
            if (valueJson.IsNumber())
            {
                *reinterpret_cast<float*>(data) = valueJson.GetFloat();
            }
            break;

        case ScriptFieldType::Int:
            if (valueJson.IsInt())
            {
                *reinterpret_cast<int*>(data) = valueJson.GetInt();
            }
            break;

        case ScriptFieldType::Bool:
            if (valueJson.IsBool())
            {
                *reinterpret_cast<bool*>(data) = valueJson.GetBool();
            }
            break;

        case ScriptFieldType::EnumInt:
            if (valueJson.IsInt())
            {
                *reinterpret_cast<int*>(data) = valueJson.GetInt();
            }
            break;

        case ScriptFieldType::Vec3:
            if (valueJson.IsArray() && valueJson.Size() == 3)
            {
                Vector3* vector = reinterpret_cast<Vector3*>(data);
                vector->x = valueJson[0].GetFloat();
                vector->y = valueJson[1].GetFloat();
                vector->z = valueJson[2].GetFloat();
            }
            break;

        case ScriptFieldType::ComponentRef:
            if (valueJson.IsUint64())
            {
                ScriptComponentRef<Component>* componentReference = reinterpret_cast<ScriptComponentRef<Component>*>(data);
                componentReference->uid = static_cast<UID>(valueJson.GetUint64());
                componentReference->component = nullptr;
            }
            break;

        case ScriptFieldType::String:
            if (valueJson.IsString())
            {
                *reinterpret_cast<std::string*>(data) = valueJson.GetString();
            }
            break;
        }
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

    for (size_t i = 0; i < fieldList.count; ++i)
    {
        const ScriptFieldInfo& field = fieldList.fields[i];

        if (field.type != ScriptFieldType::ComponentRef)
        {
            continue;
        }

        void* data = base + field.offset;
        ScriptComponentRef<Component>* componentReference = reinterpret_cast<ScriptComponentRef<Component>*>(data);

        componentReference->component = nullptr;

        if (componentReference->uid == 0)
        {
            continue;
        }

        Component* resolved = resolver.getClonedComponent(componentReference->uid);
        if (!resolved)
        {
            continue;
        }

        if (resolved->getType() == field.componentRefInfo.componentType)
        {
            componentReference->component = resolved;
        }
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

    const size_t count = sourceFields.count;

    char* sourceBase = (char*)&source;
    char* targetBase = (char*)&target;

    for (size_t i = 0; i < count; ++i)
    {
        const ScriptFieldInfo& sourceField = sourceFields.fields[i];
        const ScriptFieldInfo& targetField = targetFields.fields[i];

        void* sourceData = sourceBase + sourceField.offset;
        void* targetData = targetBase + targetField.offset;

        switch (sourceField.type)
        {
        case ScriptFieldType::Float:
            *reinterpret_cast<float*>(targetData) = *reinterpret_cast<float*>(sourceData);
            break;

        case ScriptFieldType::Int:
            *reinterpret_cast<int*>(targetData) = *reinterpret_cast<int*>(sourceData);
            break;

        case ScriptFieldType::Bool:
            *reinterpret_cast<bool*>(targetData) = *reinterpret_cast<bool*>(sourceData);
            break;

        case ScriptFieldType::EnumInt:
            *reinterpret_cast<int*>(targetData) = *reinterpret_cast<int*>(sourceData);
            break;

        case ScriptFieldType::Vec3:
            *reinterpret_cast<Vector3*>(targetData) = *reinterpret_cast<Vector3*>(sourceData);
            break;

        case ScriptFieldType::ComponentRef:
        {
            ScriptComponentRef<Component>* sourceRef = reinterpret_cast<ScriptComponentRef<Component>*>(sourceData);
            ScriptComponentRef<Component>* targetRef = reinterpret_cast<ScriptComponentRef<Component>*>(targetData);

            targetRef->uid = sourceRef->uid;
            targetRef->component = nullptr;
            break;
        }

        case ScriptFieldType::String:
            *reinterpret_cast<std::string*>(targetData) = *reinterpret_cast<std::string*>(sourceData);
            break;
        }
    }
}
