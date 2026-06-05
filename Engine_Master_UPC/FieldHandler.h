#pragma once

class IArchive;
class IFieldContainer;
class SceneReferenceResolver;

struct FieldInfo;

struct FieldHandler
{
    void (*drawUi)(const FieldInfo& field, void* data, IFieldContainer& container);
    void (*serialize)(const FieldInfo& field, const void* data, IArchive& archive);
    void (*deserialize)(const FieldInfo& field, void* data, IArchive& archive);
    void (*clone)(const FieldInfo& field, const void* sourceData, void* targetData);
    void (*fixReferences)(const FieldInfo& field, void* data, const SceneReferenceResolver& resolver);
};
