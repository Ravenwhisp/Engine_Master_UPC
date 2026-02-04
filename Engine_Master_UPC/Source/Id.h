#pragma once

struct ID_TYPE {
	UINT index : 24;
	UINT generation : 8;

	ID_TYPE() : index(0), generation(0) { ; }
	explicit ID_TYPE(UINT handle) { *reinterpret_cast<UINT*>(this) = handle; }
	operator UINT() { return *reinterpret_cast<UINT*>(this); }
};
