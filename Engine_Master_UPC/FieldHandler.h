#pragma once

#include <rapidjson/document.h>

class IFieldContainer;
class SceneReferenceResolver;
struct FieldInfo;
class IArchive;

struct FieldHandler
{
    void (*drawUi)(const FieldInfo& field, void* data, IFieldContainer& container);
    void (*serialize)(const FieldInfo& field, const void* data, IArchive& archieve);
    void (*clone)(const FieldInfo& field, const void* sourceData, void* targetData);
    void (*fixReferences)(const FieldInfo& field, void* data, const SceneReferenceResolver& resolver);
};
