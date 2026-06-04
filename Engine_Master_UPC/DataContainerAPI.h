#pragma once

// Include this header in your external project to create custom DataContainer assets
// (similar to Unity ScriptableObjects)
//
// Usage (field-based, header-only):
//   class MyConfig : public DataContainer {
//       DECLARE_DATACONTAINER(MyConfig)   // handles auto-registration
//   public:
//       MyConfig() = default;
//       explicit MyConfig(AssetReference& id) : DataContainer(id) {}
//       float m_value = 10.0f;
//
//       IMPLEMENT_DATACONTAINER_FIELDS(MyConfig,
//           SERIALIZED_FLOAT(m_value, "My Value", 0, 100, 1.0f)
//       )
//   };
//
// The class auto-registers and appears in the FileDialog "Create" menu and "Asset" top menu.
// Note: The header must be included by at least one .cpp file for registration to take effect.

#include "DataContainer.h"
#include "FieldMacros.h"
#include "EngineAPI.h"

#define DECLARE_DATACONTAINER(TypeName) \
	public: \
		static std::unique_ptr<DataContainer> CreateInstance(AssetReference& uid) { \
			return std::make_unique<TypeName>(uid); \
		} \
	private: \
		static inline const bool s_registered_ = []() -> bool { \
			::registerDataContainer(#TypeName, #TypeName, &TypeName::CreateInstance); \
			return true; \
		}();

#define IMPLEMENT_DATACONTAINER(TypeName)
