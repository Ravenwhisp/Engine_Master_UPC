#include "Globals.h"

#include "FieldHandlerRegistry.h"
#include "IFieldContainer.h"
#include "ComponentRef.h"
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
    void elementSerialize<ComponentRef<Component>>(ComponentRef<Component>& elem, IArchive& archive)
    {
        archive.serialize(elem.uid, "");
        if (archive.mode() == ArchiveMode::Input)
            elem.component = nullptr;
    }

    template<typename T>
    void drawListFieldUi(const FieldInfo& field, void* data, IFieldContainer& container)
    {
        auto* vec = reinterpret_cast<std::vector<T>*>(data);
        const FieldHandler* elemHandler = field.listInfo.elementHandler;

        std::string headerLabel = std::string(field.name) + " (" + std::to_string(vec->size()) + ")";
        ImGui::Text("%s", headerLabel.c_str());

        int removeIndex = -1;

        for (size_t i = 0; i < vec->size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));

            char elementLabel[64];
            snprintf(elementLabel, sizeof(elementLabel), "[%zu]", i);

            FieldInfo elementField = field;
            elementField.name = elementLabel;
            elementField.type = field.listInfo.elementType;
            elementField.handler = elemHandler;

            T elemCopy = (*vec)[i];
            elemHandler->drawUi(elementField, &elemCopy, container);
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
            container.onFieldEdited(field);
        }

        std::string addLabel = std::string("+ Add Element##") + field.name;
        if (ImGui::Button(addLabel.c_str()))
        {
            vec->push_back(T{});
            container.onFieldEdited(field);
        }

        ImGui::SameLine();

        std::string clearLabel = std::string("Clear All##") + field.name;
        if (ImGui::Button(clearLabel.c_str()))
        {
            if (!vec->empty())
            {
                vec->clear();
                container.onFieldEdited(field);
            }
        }
    }

    template<typename T>
    void serializeListField(const FieldInfo& field, void* data, IArchive& archive)
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
    void cloneListField(const FieldInfo& field, const void* sourceData, void* targetData)
    {
        const auto* sourceVec = reinterpret_cast<const std::vector<T>*>(sourceData);
        auto* targetVec = reinterpret_cast<std::vector<T>*>(targetData);
        const FieldHandler* elemHandler = field.listInfo.elementHandler;

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
    void fixReferencesListField(const FieldInfo& field, void* data, const SceneReferenceResolver& resolver)
    {
        auto* vec = reinterpret_cast<std::vector<T>*>(data);
        const FieldHandler* elemHandler = field.listInfo.elementHandler;

        for (size_t i = 0; i < vec->size(); ++i)
        {
            T elemCopy = (*vec)[i];
            elemHandler->fixReferences(field, &elemCopy, resolver);
            (*vec)[i] = elemCopy;
        }
    }

    // --- Template instantiations for each supported element type ---

    const FieldHandler floatListHandler = {
        &drawListFieldUi<float>,
        &serializeListField<float>,
        &cloneListField<float>,
        &fixReferencesListField<float>
    };

    const FieldHandler intListHandler = {
        &drawListFieldUi<int>,
        &serializeListField<int>,
        &cloneListField<int>,
        &fixReferencesListField<int>
    };

    const FieldHandler boolListHandler = {
        &drawListFieldUi<bool>,
        &serializeListField<bool>,
        &cloneListField<bool>,
        &fixReferencesListField<bool>
    };

    const FieldHandler vec3ListHandler = {
        &drawListFieldUi<Vector3>,
        &serializeListField<Vector3>,
        &cloneListField<Vector3>,
        &fixReferencesListField<Vector3>
    };

    const FieldHandler stringListHandler = {
        &drawListFieldUi<std::string>,
        &serializeListField<std::string>,
        &cloneListField<std::string>,
        &fixReferencesListField<std::string>
    };

    const FieldHandler enumIntListHandler = {
        &drawListFieldUi<int>,
        &serializeListField<int>,
        &cloneListField<int>,
        &fixReferencesListField<int>
    };

    const FieldHandler componentRefListHandler = {
        &drawListFieldUi<ComponentRef<Component>>,
        &serializeListField<ComponentRef<Component>>,
        &cloneListField<ComponentRef<Component>>,
        &fixReferencesListField<ComponentRef<Component>>
    };
}

const FieldHandler* getListFieldHandler(FieldType elementType)
{
    switch (elementType)
    {
        case FieldType::Float:        return &floatListHandler;
        case FieldType::Int:          return &intListHandler;
        case FieldType::Bool:         return &boolListHandler;
        case FieldType::Vec3:         return &vec3ListHandler;
        case FieldType::String:       return &stringListHandler;
        case FieldType::EnumInt:      return &enumIntListHandler;
        case FieldType::ComponentRef: return &componentRefListHandler;
        default:                      return nullptr;
    }
}
