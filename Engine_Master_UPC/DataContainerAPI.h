#pragma once

// Include this header in your external project to create custom DataContainer assets
// (similar to Unity ScriptableObjects)
//
// All DataContainer assets use the .datacontainer file extension.
// Derived classes are identified by _typeName in the JSON file.
//
// Usage:
//   .h file:
//     class MyConfig : public DataContainer {
//         DECLARE_DATACONTAINER(MyConfig)
//     public:
//         MyConfig() = default;
//         explicit MyConfig(AssetReference& id) : DataContainer(id) {}
//         bool deserializeJson(const rapidjson::Value& obj) override;
//         rapidjson::Value getJson(rapidjson::Document::AllocatorType& allocator) const override;
//         float m_value = 10.0f;
//     };
//   .cpp file:
//     IMPLEMENT_DATACONTAINER(MyConfig)
//     // override deserializeJson/getJson to serialize members
//
// The class auto-registers and appears in the FileDialog "Create" menu and "Asset" top menu.

#include "DataContainer.h"
#include "DataContainerAutoRegister.h"
#include "EngineAPI.h"