#pragma once

#include "FieldMacros.h"

#define IMPLEMENT_DATACONTAINER_FIELDS(TypeName, ...) \
	FieldList getExposedFields() const override \
	{ \
		using ThisType = TypeName; \
		static const FieldInfo ownFields[] = { __VA_ARGS__ }; \
		return FieldList(ownFields, FIELD_COUNT(ownFields)); \
	}
