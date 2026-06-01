#pragma once

#include "ScriptFieldMacros.h"

#define IMPLEMENT_DATACONTAINER_FIELDS(TypeName, ...) \
	ScriptFieldList getExposedFields() const override \
	{ \
		using ThisScript = TypeName; \
		static const ScriptFieldInfo ownFields[] = { __VA_ARGS__ }; \
		return ScriptFieldList(ownFields, SCRIPT_FIELD_COUNT(ownFields)); \
	}
