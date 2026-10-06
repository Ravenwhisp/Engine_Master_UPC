#pragma once
#include <cstdint>

// Binary MeshRenderer records already contain a uint32 render mode. Reserve
// its high bit for outline opt-out so legacy records (0, 1, 2) retain the
// default enabled outline without changing the size of any record.
namespace MeshRenderFlags
{
    constexpr uint32_t OutlineDisabled = 0x80000000u;

    constexpr uint32_t encode(uint32_t mode, bool drawOutline)
    {
        return mode | (drawOutline ? 0u : OutlineDisabled);
    }

    constexpr uint32_t mode(uint32_t flags) { return flags & ~OutlineDisabled; }
    constexpr bool drawOutline(uint32_t flags) { return (flags & OutlineDisabled) == 0; }
}
