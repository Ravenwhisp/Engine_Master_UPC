#pragma once

#include <memory>

struct AssetReference;
class DataContainer;

using DataContainerCreator = DataContainer* (*)(AssetReference& uid);
