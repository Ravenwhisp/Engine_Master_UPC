#pragma once

#include "ScriptCreator.h"
#include <string>

struct ScriptRegistry
{
    std::string name;
    ScriptCreator creator;
};