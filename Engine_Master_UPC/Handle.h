#pragma once
#include "Globals.h"

struct Handle {
	UINT index : 24;
	UINT generation : 8;

	Handle() : index(0), generation(0) { ; }
	explicit Handle(UINT handle) { *reinterpret_cast<UINT*>(this) = handle; }
	operator UINT() { return *reinterpret_cast<UINT*>(this); }
};
