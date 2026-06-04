#pragma once

#include <rapidjson/document.h>

class Script;
class ScriptComponent;
class SceneReferenceResolver;

struct FieldInfo;

struct FieldHandler
{
    void (*drawUi)(const FieldInfo& field, void* data, Script& script, ScriptComponent& owner);
    void (*serialize)(const FieldInfo& field, const void* data, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree);
    void (*deserialize)(const FieldInfo& field, void* data, const rapidjson::Value& valueJson);
    void (*clone)(const FieldInfo& field, const void* sourceData, void* targetData);
    void (*fixReferences)(const FieldInfo& field, void* data, const SceneReferenceResolver& resolver);
};
