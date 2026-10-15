// "Distance blur" (the PC menus' Display tab, off unless chosen): the levels' blur of what's far away, left out. The game
// blurs the distance as part of its fog: each frame the main loop (0x1C7F88) sets the fog with NuLightFog (0x1401F0: near,
// far, colour, blur, haze), which keeps the blur's strength as NuRndrFogBlur (0x63DA14, the blur shifted to the top byte),
// and the renderer (0x13CD48, the only reader) draws the blur when it isn't 0. The effect is drawn at the PS2's own resolution
// (as it must be: it reads the depth buffer as other pixels' bytes), so with the picture drawn larger its blurred distance is
// much softer than the sharp things in front of it. Off, NuLightFog sets everything else as the level asks and the blur 0.
#include "woc.h"

#include "ps2/runtime.h"

extern "C" void f_001401f0(Ps2Cpu* c); // NuLightFog

using namespace Woc;

namespace
{
constexpr u32 FogBlur = 0x63DA14;

bool g_blur = false;
// The level's own blur, as NuLightFog was given it last (for the setting turned on again mid-level)
s32 g_levelBlur = 0;
}

int WocDistanceBlur() { return g_blur ? 1 : 0; }

void WocSetDistanceBlur(int on)
{
    g_blur = on != 0;
    Write32(FogBlur, g_blur ? g_levelBlur : 0);
}

extern "C" void WocNuLightFog(Ps2Cpu* c)
{
    f_001401f0(c);
    g_levelBlur = Read32(FogBlur);
    if (!g_blur)
    {
        Write32(FogBlur, 0);
    }
}
