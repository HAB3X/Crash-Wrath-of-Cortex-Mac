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

// The game's pause (USA v1.01): Paused at 0x63F70C counts up to 30 frames as it pauses and back down as it goes on, the way it's
// going at 0x63F710 (1 pausing, 2 going on, 0 neither); the pause menu is menu 4 (RESUME, OPTIONS, QUIT) of the cursor at
// 0x5A4380 (its menu at +0x5E), its options menu 5. The PC menus open from those options (src/menus.cpp), so what they do in
// play (src/levels.cpp, src/saving.cpp) is done after the game goes on, as its RESUME does it (0x63F710 set to 2)
constexpr u32 PausedFrames = 0x63F70C;
constexpr u32 PauseWay = 0x63F710;

// Paused all the way, its pause menu or the options up
inline bool InPauseMenu()
{
    s8 menu = static_cast<s8>(*At(0x5A4380 + 0x5E));
    return Read32(PausedFrames) == 30 && Read32(PauseWay) == 0 && (menu == 4 || menu == 5);
}

// The game goes on as RESUME has it
inline void Resume() { Write32(PauseWay, 2); }

// Still going back from the pause
inline bool Resuming() { return Read32(PausedFrames) != 0 && Read32(PauseWay) == 2; }

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
