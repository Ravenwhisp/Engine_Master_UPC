#pragma once

class Script;

struct ScriptMethodInfo
{
    const char* name;
    void(*func)(Script*);
};
