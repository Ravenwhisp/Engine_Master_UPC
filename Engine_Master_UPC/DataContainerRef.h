#pragma once

#include "UID.h"
#include <memory>

class DataContainer;

template<typename T = DataContainer>
struct DataContainerRef
{
    UID uid = INVALID_UID;
    std::shared_ptr<DataContainer> dataContainer;

    T* get() const
    {
        return static_cast<T*>(dataContainer.get());
    }
};
