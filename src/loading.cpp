// Faster loading (the "Faster loading" setting, on by default): a level's loading screen ends as soon as the level is loaded.
//
// A level loads on a thread of its own (0x1CA908 and its kin set 0x6417F8 once it's done) while the loading screen (0x1C6368,
// Crash falling through the vortex) plays. That screen goes through its own steps first, Crash and the camera gliding into
// place (its state, a word on its stack at sp+0x4120: 0 and 1), and only in its third step (2) looks at whether the loading is
// done; then it fades to black (fade_rate 8) and leaves (step 3). Here the loading takes about a second, so most of the
// screen's four seconds were its first steps. With the setting on, once the loading is done the screen takes its last step at
// once, as its third step would: step 3 and the fade. The cut movies' loading screen (0x1C7548) and the logos' already end as
// soon as their loading is done.
//
// It's done from the loading screen's call of the sound effects' frame (0x248750, overridden: src/options.cpp), called every
// frame from 0x1C66C0 with the loading screen's own stack pointer, so its state's place is known.
//
// The game's state (USA v1.01): the loading thread's flag 0x6417F8, fade_rate 0x63E694.
#include "woc.h"

using namespace Woc;

namespace
{
constexpr u32 LoadingScreenCall = 0x1C66C8; // the return address of the loading screen's call
constexpr u32 StateOffset = 0x4120;
constexpr s32 LastStep = 3;
constexpr u32 Loaded = 0x6417F8;
constexpr u32 FadeRate = 0x63E694;

bool g_enabled = true;
}

int WocFasterLoading() { return g_enabled ? 1 : 0; }
void WocSetFasterLoading(int on) { g_enabled = on != 0; }

void WocLoadingFrame(Ps2Cpu* c)
{
    if (!g_enabled || c->r[31].uw[0] != LoadingScreenCall || Read32(Loaded) == 0)
    {
        return;
    }
    u32 state = c->r[29].uw[0] + StateOffset;
    s32 step = Read32(state);
    if (step >= 0 && step < LastStep)
    {
        Ps2::Log("faster loading: the loading screen ends at its step %d", step);
        Write32(state, LastStep);
        Write32(FadeRate, 8);
    }
}
