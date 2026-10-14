// Crash Bandicoot: The Wrath of Cortex for Mac: the recompiled executable on the PS2 runtime
#include "ps2/runtime.h"

void WocRegisterIopServers();
void WocPcText();
int WocTyping();
void WocVblank();
int WocMenu();
const Ps2GameSetting* WocSettings();
int WocSettingCount();
const Ps2GameAction* WocActions();
int WocActionCount();

// Before the boot: the game's IOP servers, and its texts said as a computer's
static void WocInit()
{
    WocRegisterIopServers();
    WocPcText();
}

int main(int argc, char** argv)
{
    // The PC menus: the game's settings (src/options.cpp), saving from the pause menu (src/saving.cpp) and the level select in
    // Extras (src/levels.cpp)
    static const Ps2GameInfo game = {"Crash Bandicoot: The Wrath of Cortex (USA v1.01)",
                                     "SLUS_202.38",
                                     5596300,
                                     WocInit,
                                     WocTyping,
                                     WocVblank,
                                     WocMenu,
                                     WocSettings(),
                                     WocSettingCount(),
                                     WocActions(),
                                     WocActionCount(),
                                     "Crash Bandicoot The Wrath of Cortex"};
    return ps2_main(argc, argv, &game);
}
