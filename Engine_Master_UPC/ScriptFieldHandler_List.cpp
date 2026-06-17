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
    template<typename T>
    void elementSerialize(T& elem, IArchive& archive)
    {
        archive.serialize(elem, "");
    }

    template<>
    void elementSerialize<int>(int& elem, IArchive& archive)
    {
        uint32_t v = archive.mode() == ArchiveMode::Output ? static_cast<uint32_t>(elem) : 0;
        archive.serialize(v, "");
        if (archive.mode() == ArchiveMode::Input)
            elem = static_cast<int>(v);
    }

    template<>
    void elementSerialize<Vector3>(Vector3& elem, IArchive& archive)
    {
        DirectX::SimpleMath::Vector3 v(elem.x, elem.y, elem.z);
        archive.serialize(v, "");
        if (archive.mode() == ArchiveMode::Input)
        {
            elem.x = v.x; elem.y = v.y; elem.z = v.z;
        }
    }

    template<>
    void elementSerialize<std::string>(std::string& elem, IArchive& archive)
    {
        archive.serialize(elem, "");
    }

    template<>
    void elementSerialize<ScriptComponentRef<Component>>(ScriptComponentRef<Component>& elem, IArchive& archive)
    {
        archive.serialize(elem.uid, "");
        if (archive.mode() == ArchiveMode::Input)
            elem.component = nullptr;
    }

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
    void serializeListField(const ScriptFieldInfo& field, void* data, IArchive& archive)
    {
        auto* vec = reinterpret_cast<std::vector<T>*>(data);
        uint32_t count = archive.mode() == ArchiveMode::Output ? static_cast<uint32_t>(vec->size()) : 0;
        archive.beginArray(count, field.name);

        if (archive.mode() == ArchiveMode::Input)
        {
            vec->clear();
            vec->reserve(count);
        }

        for (uint32_t i = 0; i < count; ++i)
        {
            if (archive.mode() == ArchiveMode::Output)
            {
                T elem = (*vec)[i];
                elementSerialize(elem, archive);
            }
            else
            {
                T elem{};
                elementSerialize(elem, archive);
                vec->push_back(std::move(elem));
            }
        }

        archive.endArray();
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
        &cloneListField<float>,
        &fixReferencesListField<float>
    };

    const ScriptFieldHandler intListHandler = {
        &drawListFieldUi<int>,
        &serializeListField<int>,
        &cloneListField<int>,
        &fixReferencesListField<int>
    };

    const ScriptFieldHandler boolListHandler = {
        &drawListFieldUi<bool>,
        &serializeListField<bool>,
        &cloneListField<bool>,
        &fixReferencesListField<bool>
    };

    const ScriptFieldHandler vec3ListHandler = {
        &drawListFieldUi<Vector3>,
        &serializeListField<Vector3>,
        &cloneListField<Vector3>,
        &fixReferencesListField<Vector3>
    };

    const ScriptFieldHandler stringListHandler = {
        &drawListFieldUi<std::string>,
        &serializeListField<std::string>,
        &cloneListField<std::string>,
        &fixReferencesListField<std::string>
    };

    const ScriptFieldHandler enumIntListHandler = {
        &drawListFieldUi<int>,
        &serializeListField<int>,
        &cloneListField<int>,
        &fixReferencesListField<int>
    };

    const ScriptFieldHandler componentRefListHandler = {
        &drawListFieldUi<ScriptComponentRef<Component>>,
        &serializeListField<ScriptComponentRef<Component>>,
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
