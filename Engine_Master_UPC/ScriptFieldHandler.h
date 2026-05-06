#pragma once

#include <rapidjson/document.h>

class Script;
class ScriptComponent;
class SceneReferenceResolver;

struct ScriptFieldInfo;

struct ScriptFieldHandler
{
    void (*drawUi)(const ScriptFieldInfo& field, void* data, Script& script, ScriptComponent& owner);
    void (*serialize)(const ScriptFieldInfo& field, const void* data, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree);
    void (*deserialize)(const ScriptFieldInfo& field, void* data, const rapidjson::Value& valueJson);
    void (*clone)(const ScriptFieldInfo& field, const void* sourceData, void* targetData);
    void (*fixReferences)(const ScriptFieldInfo& field, void* data, const SceneReferenceResolver& resolver);
};