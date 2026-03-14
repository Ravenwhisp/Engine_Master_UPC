#include "pch.h"
#include "GameScriptsRegistration.h"
#include "Test.h"

const char* GetScriptName()
{
    return "Test";
}

ScriptCreator GetScriptCreator()
{
    return &Test::Create;
}