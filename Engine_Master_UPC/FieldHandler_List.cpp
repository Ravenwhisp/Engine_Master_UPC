#include "Globals.h"

#include "FieldHandlerRegistry.h"
#include "IArchive.h"
#include "IFieldContainer.h"
#include "ComponentRef.h"
#include "AssetReference.h"
#include "SceneReferenceResolver.h"

#include "Application.h"
#include "ModuleScene.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"

namespace
{
    // --- Element-to-archive / archive-to-Element helpers ---

    template<typename T>
    void elementSerialize(const T& element, IArchive& archive);

    template<>
    void elementSerialize<float>(const float& element, IArchive& archive)
    {
        float val = element;
        archive.serialize(val);
    }

    template<>
    void elementSerialize<int>(const int& element, IArchive& archive)
    {
        uint32_t val = static_cast<uint32_t>(element);
        archive.serialize(val);
    }

    template<>
    void elementSerialize<bool>(const bool& element, IArchive& archive)
    {
        bool val = element;
        archive.serialize(val);
    }

    template<>
    void elementSerialize<Vector3>(const Vector3& element, IArchive& archive)
    {
        Vector3 val = element;
        archive.serialize(val);
    }

    template<>
    void elementSerialize<std::string>(const std::string& element, IArchive& archive)
    {
        std::string val = element;
        archive.serialize(val);
    }

    template<>
    void elementSerialize<ComponentRef<Component>>(const ComponentRef<Component>& element, IArchive& archive)
    {
        uint64_t val = static_cast<uint64_t>(element.uid);
        archive.serialize(val);
    }

    template<typename T>
    void elementDeserialize(T& element, IArchive& archive);

    template<>
    void elementDeserialize<float>(float& element, IArchive& archive)
    {
        archive.serialize(element);
    }

    template<>
    void elementDeserialize<int>(int& element, IArchive& archive)
    {
        uint32_t raw = static_cast<uint32_t>(element);
        archive.serialize(raw);
    }

    template<>
    void elementDeserialize<bool>(bool& element, IArchive& archive)
    {
        archive.serialize(element);
    }

    template<>
    void elementDeserialize<Vector3>(Vector3& element, IArchive& archive)
    {
        archive.serialize(element);
    }

    template<>
    void elementDeserialize<std::string>(std::string& element, IArchive& archive)
    {
        archive.serialize(element);
    }

    template<>
    void elementDeserialize<ComponentRef<Component>>(ComponentRef<Component>& element, IArchive& archive)
    {
        uint64_t val = static_cast<uint64_t>(element.uid);
        archive.serialize(val);
        element.uid = static_cast<UID>(val);
        element.component = nullptr;
    }

    template<>
    void elementSerialize<AssetReference<void>>(const AssetReference<void>& element, IArchive& archive)
    {
        archive.beginObject();
        element.m_id.serialize(archive);
        archive.endObject();
    }

    template<>
    void elementDeserialize<AssetReference<void>>(AssetReference<void>& element, IArchive& archive)
    {
        archive.beginObject();
        element.m_id.serialize(archive);
        archive.endObject();
        element.m_data = nullptr;
    }

    // --- Generic list handler templates ---

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
        if (archive.mode() == ArchiveMode::Output)
        {
            const auto* vec = reinterpret_cast<const std::vector<T>*>(data);

            uint32_t count = static_cast<uint32_t>(vec->size());
            archive.beginArray(count, field.name);

            for (const T& elem : *vec)
            {
                elementSerialize(elem, archive);
            }

            archive.endArray();
        }
        else
        {
            auto* vec = reinterpret_cast<std::vector<T>*>(data);
            vec->clear();

            uint32_t count = 0;
            archive.beginArray(count, field.name);
            vec->reserve(count);

            for (uint32_t i = 0; i < count; ++i)
            {
                T elem{};
                elementDeserialize(elem, archive);
                vec->push_back(std::move(elem));
            }

            archive.endArray();
        }
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

    const FieldHandler assetRefListHandler = {
        &drawListFieldUi<AssetReference<void>>,
        &serializeListField<AssetReference<void>>,
        &cloneListField<AssetReference<void>>,
        &fixReferencesListField<AssetReference<void>>
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
        case FieldType::AssetRef:     return &assetRefListHandler;
        default:                      return nullptr;
    }
}
