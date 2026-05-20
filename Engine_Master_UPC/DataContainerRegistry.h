#pragma once

#include "DataContainerCreator.h"
#include <string>

struct DataContainerRegistry
{
    std::string name;
    std::string displayName;
    DataContainerCreator creator;
};