#pragma once

class Script;
class ScriptComponent;
class SceneReferenceResolver;
class IArchive;

struct ScriptFieldInfo;

struct ScriptFieldHandler
{
    void (*drawUi)(const ScriptFieldInfo& field, void* data, Script& script, ScriptComponent& owner);
    void (*serialize)(const ScriptFieldInfo& field, void* data, IArchive& archive);
    void (*clone)(const ScriptFieldInfo& field, const void* sourceData, void* targetData);
    void (*fixReferences)(const ScriptFieldInfo& field, void* data, const SceneReferenceResolver& resolver);
};