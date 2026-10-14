// What the game-specific files share: the game's memory read and written by address, and the game's own functions called from
// an override (the registers kept as they were, so the overridden function's arguments and the caller's state survive)
#pragma once

#include "ps2/iop.h"
#include "ps2/recomp.h"

#include <cstring>
#include <initializer_list>

namespace Woc
{
inline u8* At(u32 address) { return Ps2::EeMemory(address); }

inline s32 Read32(u32 address)
{
    s32 v;
    std::memcpy(&v, At(address), 4);
    return v;
}

inline void Write32(u32 address, s32 v) { std::memcpy(At(address), &v, 4); }

inline float ReadFloat(u32 address)
{
    float v;
    std::memcpy(&v, At(address), 4);
    return v;
}

inline void WriteFloat(u32 address, float v) { std::memcpy(At(address), &v, 4); }

inline s8 Read8(u32 address) { return static_cast<s8>(*At(address)); }

inline void Write8(u32 address, u8 v) { *At(address) = v; }

// Calls one of the game's functions with integer arguments (a0-a3) from an override, on that override's context: everything in
// the registers is put back afterwards, as if the call hadn't happened (only memory changes)
inline void CallGame(Ps2Cpu* c, Ps2Function function, std::initializer_list<s32> arguments)
{
    Ps2Cpu saved = *c;
    int i = 4;
    for (s32 a : arguments)
    {
        c->r[i++].sd[0] = a;
    }
    function(c);
    *c = saved;
}
}
