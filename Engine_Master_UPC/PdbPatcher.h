#pragma once

#include <string>

class PdbPatcher
{
public:
    static bool patchRuntimeDllPdbPath(const std::string& runtimeDllPath, const std::string& runtimePdbPath);
};