// Crash Bandicoot: The Wrath of Cortex for Mac: the recompiled executable on the PS2 runtime
#include "ps2/runtime.h"

void WocRegisterIopServers();
void WocPcText();
int WocTyping();
void WocVblank();
int WocMenu();

// Before the boot: the game's IOP servers, and its texts said as a computer's
static void WocInit()
{
    WocRegisterIopServers();
    WocPcText();
}

int main(int argc, char** argv)
{
    static const Ps2GameInfo game = {"Crash Bandicoot: The Wrath of Cortex (USA v1.01)", "SLUS_202.38", 5596300,
                                     WocInit, WocTyping, WocVblank, WocMenu};
    return ps2_main(argc, argv, &game);
}
