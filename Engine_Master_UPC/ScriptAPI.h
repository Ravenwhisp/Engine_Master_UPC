#pragma once

#include "Script.h"
#include "GameObject.h"
#include "Transform.h"

#include "GenericTypeFactory.h"
#include "ScriptFieldInfo.h"
#include "ScriptFieldMacros.h"

#include "ScriptComponentRef.h"
#include "ScriptDataContainerRef.h"

#define DECLARE_SCRIPT(ScriptType)

#define IMPLEMENT_SCRIPT(ScriptType) \
	namespace { \
		static std::unique_ptr<Script> s_create_##ScriptType(GameObject* owner) { \
			return std::make_unique<ScriptType>(owner); \
		} \
		static bool s_registered_##ScriptType = ( \
			::registerScript(#ScriptType, &s_create_##ScriptType), \
			true \
		); \
	}