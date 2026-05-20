#pragma once

// Include this header in your external project to create custom DataContainer assets
// (similar to Unity ScriptableObjects)

#include "DataContainer.h"
#include "DataContainerAutoRegister.h"
#include "EngineAPI.h"

// Usage example:
//
// In your .h file:
// class MyConfig : public DataContainer {
// public:
//     DECLARE_DATACONTAINER(MyConfig)
//     MyConfig() = default;
//     explicit MyConfig(AssetReference& id) : DataContainer(id) {}
//     const char* getTypeName() const override { return "MyConfig"; }
//     const char* getDisplayTypeName() const override { return "My Config"; }
//     const char* getFileExtension() const override { return ".myconfig"; }
//     // Add your typed accessor methods here...
// };
//
// In your .cpp file:
// IMPLEMENT_DATACONTAINER(MyConfig, ".myconfig")
//
// The class will auto-register and appear in the FileDialog right-click "Create" menu.
