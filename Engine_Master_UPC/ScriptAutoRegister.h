#pragma once

#include "EngineAPI.h"

class ScriptAutoRegister
{
public:
    ScriptAutoRegister(const char* name, ScriptCreator creator)
    {
        registerScript(name, creator);
    }
};

#define DECLARE_SCRIPT(ScriptType) \
public: \
    static std::unique_ptr<Script> Create(GameObject* owner);

#define IMPLEMENT_SCRIPT(ScriptType)                                      \
    std::unique_ptr<Script> ScriptType::Create(GameObject* owner)         \
    {                                                                     \
        return std::make_unique<ScriptType>(owner);                       \
    }                                                                     \
    static ScriptAutoRegister s_autoRegister_##ScriptType(                \
        #ScriptType,                                                      \
        &ScriptType::Create                                               \
    );