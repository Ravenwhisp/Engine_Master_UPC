#pragma once

#include <memory>

class AssetReference;
class DataContainer;

using DataContainerCreator = DataContainer* (*)(AssetReference& uid);
