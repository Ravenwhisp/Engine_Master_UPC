#pragma once

#include "DeviceType.h"

struct PlayerBinding
{
    DeviceType deviceType = DeviceType::None;
    int deviceIndex = 0;
};