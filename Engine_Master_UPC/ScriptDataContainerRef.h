#pragma once

#include "UID.h"
#include <memory>

class DataContainer;

struct ScriptDataContainerRef
{
    UID uid = INVALID_UID;
    std::shared_ptr<DataContainer> dataContainer;
};
