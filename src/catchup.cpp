// Late frames made up as on the console. The game's main loop (0x1C7F88) counts the vertical blanks each frame took
// (nuvideo_global_vbcnt 0x63DC34 against the last frame's count; frameout 0x63E710, at most 2) and runs that many more steps
// of play in the next frame (FRAMES 0x63E718), so its time keeps up with the TV's. After 6 late frames in a row it asks its
// render timers (tbslotLastValue 0x16A780 on the default set's slots, in timer 1's horizontal blanks: 1 the frame's VIF1
// transfer, from NuRndrVu1Draw starting it to its last tag's interrupt; 0 from that start to the swap's wait for it) whether
// drawing took a field; when neither took 235 lines, it decides drawing didn't make the frames late, and runs one step a frame
// again (frameout 0 at 0x1C9CA8) until a frame is on time.
//
// Here the transfer ends as it starts (the render thread draws it afterwards, holding the game's thread back at the vertical
// blank when it's behind), so the timers read a few lines and the game always took the second way: in a level drawn at 30
// frames a second it ran one step a frame, at half speed. Those two calls are the function's only ones; they're answered as
// drawing taking a whole field (262 lines), and the game makes up its late frames whatever makes them late.
#include "woc.h"

namespace
{
constexpr s32 FieldLines = 262;
}

extern "C" void WocRenderTime(Ps2Cpu* c) { c->r[2].sd[0] = FieldLines; }

