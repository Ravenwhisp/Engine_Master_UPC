#include "Globals.h"

#include "ScriptFieldHandlerRegistry.h"
#include "Script.h"
#include "ScriptComponentRef.h"
#include "SceneReferenceResolver.h"

#include "Application.h"
#include "ModuleScene.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"

namespace
{
    // --- Element-to-JSON / JSON-to-Element helpers ---

    template<typename T>
    rapidjson::Value elementToJson(const T& element, rapidjson::Document& domTree);

    template<>
    rapidjson::Value elementToJson<float>(const float& element, rapidjson::Document&)
    {
        return rapidjson::Value(element);
    }

    template<>
    rapidjson::Value elementToJson<int>(const int& element, rapidjson::Document&)
    {
        return rapidjson::Value(element);
    }

    template<>
    rapidjson::Value elementToJson<bool>(const bool& element, rapidjson::Document&)
    {
        return rapidjson::Value(element);
    }

    template<>
    rapidjson::Value elementToJson<Vector3>(const Vector3& element, rapidjson::Document& domTree)
    {
        rapidjson::Value array(rapidjson::kArrayType);
        array.PushBack(element.x, domTree.GetAllocator());
        array.PushBack(element.y, domTree.GetAllocator());
        array.PushBack(element.z, domTree.GetAllocator());
        return array;
    }

    template<>
    rapidjson::Value elementToJson<std::string>(const std::string& element, rapidjson::Document& domTree)
    {
        return rapidjson::Value(element.c_str(), domTree.GetAllocator());
    }

    template<>
    rapidjson::Value elementToJson<ScriptComponentRef<Component>>(const ScriptComponentRef<Component>& element, rapidjson::Document&)
    {
        return rapidjson::Value(static_cast<uint64_t>(element.uid));
    }

    template<typename T>
    void elementFromJson(T& element, const rapidjson::Value& json);

    template<>
    void elementFromJson<float>(float& element, const rapidjson::Value& json)
    {
        if (json.IsNumber()) element = json.GetFloat();
    }

    template<>
    void elementFromJson<int>(int& element, const rapidjson::Value& json)
    {
        if (json.IsInt()) element = json.GetInt();
    }

    template<>
    void elementFromJson<bool>(bool& element, const rapidjson::Value& json)
    {
        if (json.IsBool()) element = json.GetBool();
    }

    template<>
    void elementFromJson<Vector3>(Vector3& element, const rapidjson::Value& json)
    {
        if (json.IsArray() && json.Size() == 3)
        {
            element.x = json[0].GetFloat();
            element.y = json[1].GetFloat();
            element.z = json[2].GetFloat();
        }
    }

    template<>
    void elementFromJson<std::string>(std::string& element, const rapidjson::Value& json)
    {
        if (json.IsString()) element = json.GetString();
    }

    template<>
    void elementFromJson<ScriptComponentRef<Component>>(ScriptComponentRef<Component>& element, const rapidjson::Value& json)
    {
        if (json.IsUint64())
        {
            element.uid = static_cast<UID>(json.GetUint64());
        }
        element.component = nullptr;
    }

    // --- Generic list handler templates ---

    template<typename T>
    void drawListFieldUi(const ScriptFieldInfo& field, void* data, Script& script, ScriptComponent& owner)
    {
        auto* vec = reinterpret_cast<std::vector<T>*>(data);
        const ScriptFieldHandler* elemHandler = field.listInfo.elementHandler;

        std::string headerLabel = std::string(field.name) + " (" + std::to_string(vec->size()) + ")";
        ImGui::Text("%s", headerLabel.c_str());

        int removeIndex = -1;

        for (size_t i = 0; i < vec->size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));

            char elementLabel[64];
            snprintf(elementLabel, sizeof(elementLabel), "[%zu]", i);

            ScriptFieldInfo elementField = field;
            elementField.name = elementLabel;
            elementField.type = field.listInfo.elementType;
            elementField.handler = elemHandler;

            T elemCopy = (*vec)[i];
            elemHandler->drawUi(elementField, &elemCopy, script, owner);
            (*vec)[i] = elemCopy;

            ImGui::SameLine();

            if (ImGui::Button("X"))
            {
                removeIndex = static_cast<int>(i);
            }

            ImGui::PopID();
        }

        if (removeIndex >= 0)
        {
            vec->erase(vec->begin() + removeIndex);
            script.onFieldEdited(field);
        }

        std::string addLabel = std::string("+ Add Element##") + field.name;
        if (ImGui::Button(addLabel.c_str()))
        {
            vec->push_back(T{});
            script.onFieldEdited(field);
        }

        ImGui::SameLine();

        std::string clearLabel = std::string("Clear All##") + field.name;
        if (ImGui::Button(clearLabel.c_str()))
        {
            if (!vec->empty())
            {
                vec->clear();
                script.onFieldEdited(field);
            }
        }
    }

    template<typename T>
    void serializeListField(const ScriptFieldInfo& field, const void* data, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree)
    {
        const auto* vec = reinterpret_cast<const std::vector<T>*>(data);

        rapidjson::Value key(field.name, domTree.GetAllocator());
        rapidjson::Value array(rapidjson::kArrayType);

        for (const T& elem : *vec)
        {
            array.PushBack(elementToJson(elem, domTree), domTree.GetAllocator());
        }

        outFieldsJson.AddMember(key, array, domTree.GetAllocator());
    }

    template<typename T>
    void deserializeListField(const ScriptFieldInfo&, void* data, const rapidjson::Value& valueJson)
    {
        if (!valueJson.IsArray())
        {
            return;
        }

        auto* vec = reinterpret_cast<std::vector<T>*>(data);
        vec->clear();
        vec->reserve(valueJson.Size());

        for (rapidjson::SizeType i = 0; i < valueJson.Size(); ++i)
        {
            T elem{};
            elementFromJson(elem, valueJson[i]);
            vec->push_back(std::move(elem));
        }
    }

    template<typename T>
    void cloneListField(const ScriptFieldInfo& field, const void* sourceData, void* targetData)
    {
        const auto* sourceVec = reinterpret_cast<const std::vector<T>*>(sourceData);
        auto* targetVec = reinterpret_cast<std::vector<T>*>(targetData);
        const ScriptFieldHandler* elemHandler = field.listInfo.elementHandler;

        targetVec->clear();
        targetVec->reserve(sourceVec->size());

        for (const T& sourceElem : *sourceVec)
        {
            T targetElem;
            elemHandler->clone(field, &sourceElem, &targetElem);
            targetVec->push_back(std::move(targetElem));
        }
    }

    template<typename T>
    void fixReferencesListField(const ScriptFieldInfo& field, void* data, const SceneReferenceResolver& resolver)
    {
        auto* vec = reinterpret_cast<std::vector<T>*>(data);
        const ScriptFieldHandler* elemHandler = field.listInfo.elementHandler;

        for (size_t i = 0; i < vec->size(); ++i)
        {
            T elemCopy = (*vec)[i];
            elemHandler->fixReferences(field, &elemCopy, resolver);
            (*vec)[i] = elemCopy;
        }
    }

    // --- Template instantiations for each supported element type ---

    const ScriptFieldHandler floatListHandler = {
        &drawListFieldUi<float>,
        &serializeListField<float>,
        &deserializeListField<float>,
        &cloneListField<float>,
        &fixReferencesListField<float>
    };

    const ScriptFieldHandler intListHandler = {
        &drawListFieldUi<int>,
        &serializeListField<int>,
        &deserializeListField<int>,
        &cloneListField<int>,
        &fixReferencesListField<int>
    };

    const ScriptFieldHandler boolListHandler = {
        &drawListFieldUi<bool>,
        &serializeListField<bool>,
        &deserializeListField<bool>,
        &cloneListField<bool>,
        &fixReferencesListField<bool>
    };

    const ScriptFieldHandler vec3ListHandler = {
        &drawListFieldUi<Vector3>,
        &serializeListField<Vector3>,
        &deserializeListField<Vector3>,
        &cloneListField<Vector3>,
        &fixReferencesListField<Vector3>
    };

    const ScriptFieldHandler stringListHandler = {
        &drawListFieldUi<std::string>,
        &serializeListField<std::string>,
        &deserializeListField<std::string>,
        &cloneListField<std::string>,
        &fixReferencesListField<std::string>
    };

    const ScriptFieldHandler enumIntListHandler = {
        &drawListFieldUi<int>,
        &serializeListField<int>,
        &deserializeListField<int>,
        &cloneListField<int>,
        &fixReferencesListField<int>
    };

    const ScriptFieldHandler componentRefListHandler = {
        &drawListFieldUi<ScriptComponentRef<Component>>,
        &serializeListField<ScriptComponentRef<Component>>,
        &deserializeListField<ScriptComponentRef<Component>>,
        &cloneListField<ScriptComponentRef<Component>>,
        &fixReferencesListField<ScriptComponentRef<Component>>
    };
}

const ScriptFieldHandler* getListFieldHandler(ScriptFieldType elementType)
{
    switch (elementType)
    {
        case ScriptFieldType::Float:        return &floatListHandler;
        case ScriptFieldType::Int:          return &intListHandler;
        case ScriptFieldType::Bool:         return &boolListHandler;
        case ScriptFieldType::Vec3:         return &vec3ListHandler;
        case ScriptFieldType::String:       return &stringListHandler;
        case ScriptFieldType::EnumInt:      return &enumIntListHandler;
        case ScriptFieldType::ComponentRef: return &componentRefListHandler;
        default:                            return nullptr;
    }
}
