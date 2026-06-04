#pragma once

// Include this header in your external project to create custom DataContainer assets
// (similar to Unity ScriptableObjects)
//
// Usage (field-based, recommended, header-only):
//   class MyConfig : public DataContainer {
//       DECLARE_DATACONTAINER(MyConfig)
//   public:
//       MyConfig() = default;
//       explicit MyConfig(AssetReference& id) : DataContainer(id) {}
//       float m_value = 10.0f;
//
//       IMPLEMENT_DATACONTAINER_FIELDS(MyConfig,
//           SERIALIZED_FLOAT(m_value, "My Value", 0, 100, 1.0f)
//       )
//   };
//   IMPLEMENT_DATACONTAINER(MyConfig)
//
// The class auto-registers and appears in the FileDialog "Create" menu and "Asset" top menu.

#include "DataContainer.h"
#include "FieldMacros.h"
#include "EngineAPI.h"

#define DECLARE_DATACONTAINER(TypeName)

#define IMPLEMENT_DATACONTAINER(TypeName) \
	namespace { \
		static std::unique_ptr<DataContainer> s_create_##TypeName(AssetReference& uid) { \
			return std::make_unique<TypeName>(uid); \
		} \
		static bool s_registered_##TypeName = ( \
			::registerDataContainer(#TypeName, #TypeName, &s_create_##TypeName), \
			true \
		); \
	}
