#pragma once
#include <cstddef>

namespace PlatformOffsets
{
// Offset of CGameEntitySystem* inside GameResourceServiceServerV001.
// Verified against current CounterStrikeSharp gamedata and multiple native CS2 plugins
// on 2026-09-10. Keep this isolated because Valve can change it independently of schema.
#ifdef _WIN32
inline constexpr std::ptrdiff_t kGameEntitySystem = 0x58;
#else
inline constexpr std::ptrdiff_t kGameEntitySystem = 0x50;
#endif
}
