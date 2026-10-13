// Crash Bandicoot: The Wrath of Cortex for Mac: the recompiled executable on the PS2 runtime
#include "ps2/runtime.h"

void WocRegisterIopServers();
int WocTyping();
void WocVblank();

int main(int argc, char** argv)
{
    static const Ps2GameInfo game = {"Crash Bandicoot: The Wrath of Cortex (USA v1.01)", "SLUS_202.38", 5596300,
                                     WocRegisterIopServers, WocTyping, WocVblank};
    return ps2_main(argc, argv, &game);
}
