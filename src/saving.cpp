// Saving as a PC game's. The save is the game's own (its four slots in one file, written to the save folder as to the memory
// card), and so are its screens, said as a computer's (PC_TEXT.md), with two changes:
//
// The saves are read without the console's wait: opening the load or save screens reads the save file (UpdateSaveSlots,
// 0x253880, every frame of those screens: the read started with memcard_loadattempted at 0x641790 set, done when
// memcard_loadcompleted at 0x641794 is) and shows CHECKING SAVE DATA for at least 40 frames (memcard_loadmessage_delay at
// 0x6417C0), the time a memory card took. Here that wait ends as soon as the read is done, so the slots come up at once. The
// SAVING message and the result (GAME SAVED, 0x6417C4 and 0x6417D0) still stay as long as on the PS2: they say what happened.
// The card itself is always in and formatted (the runtime's), so the game's "insert a memory card" and "format" screens never
// come up, and its check at start-up (menu 2 over the title) passes at once.
//
// Saving from the PC pause menu: "Save game" (in the Warp Room, where the game itself saves) opens the game's own load and save
// screen, as walking up to the Warp Room's save point opens it (0x1D0FB0: NewMenu(cursor, 20, row, -1) and the "yes" sound,
// GameSfx 0x36), with SAVE GAME (its second row) chosen. The game's own screens do the rest: the save's slots, the
// confirmation, the save written to the save folder (the memory card's files) and their messages, said as a computer's
// (PC_TEXT.md).
//
// The game's state (USA v1.01): Level 0x63F7B0 (0x25 the Warp Room), new_level 0x63F7B4, new_mode 0x63F96C, GameMode
// 0x63FD0C, Paused 0x63F70C, cutmovie 0x641844, fadeval 0x63E68C, the menu cursor 0x5A4380 (its menu at +0x5E), NewMenu
// 0x1E2910, GameSfx 0x2487D8.
#include "woc.h"

#include "ps2/runtime.h"

extern "C" void f_001e2910(Ps2Cpu* c); // NewMenu(cursor, menu, row, level)
extern "C" void f_002487d8(Ps2Cpu* c); // GameSfx(sound, position)
extern "C" void f_00253880(Ps2Cpu* c); // UpdateSaveSlots

using namespace Woc;

namespace
{
constexpr u32 Level = 0x63F7B0;
constexpr u32 NewLevel = 0x63F7B4;
constexpr u32 NewMode = 0x63F96C;
constexpr u32 GameMode = 0x63FD0C;
constexpr u32 Paused = 0x63F70C;
constexpr u32 CutMovie = 0x641844;
constexpr u32 FadeValue = 0x63E68C;
constexpr u32 Cursor = 0x5A4380;
constexpr s32 WarpRoom = 0x25;
constexpr s32 LoadSaveMenu = 20;
constexpr s32 SaveGameRow = 1;
constexpr s32 YesSound = 0x36;
constexpr u32 LoadAttempted = 0x641790;
constexpr u32 LoadCompleted = 0x641794;
constexpr u32 LoadMessageDelay = 0x6417C0;

bool g_open = false;

int CanSave()
{
    return Read32(Level) == WarpRoom && Read32(CutMovie) == -1 && Read32(GameMode) == 0 && Read32(NewLevel) == -1 &&
                   Read32(NewMode) == -1 && Read32(FadeValue) == 0 && Read32(Paused) == 0 && Read8(Cursor + 0x5E) == -1
               ? 1
               : 0;
}

void Save()
{
    if (CanSave() != 0)
    {
        g_open = true;
    }
}

const Ps2GameAction Actions[] = {
    {"Pause", nullptr, "Save game", CanSave, Save},
};
}

const Ps2GameAction* WocSavingActions() { return Actions; }
int WocSavingActionCount() { return sizeof(Actions) / sizeof(Actions[0]); }

// Every frame (src/options.cpp): the save screen opened when it was asked for
void WocSavingFrame(Ps2Cpu* c)
{
    if (!g_open)
    {
        return;
    }
    g_open = false;
    if (CanSave() != 0)
    {
        Ps2::Log("save game: the save screen opened");
        CallGame(c, f_001e2910, {static_cast<s32>(Cursor), LoadSaveMenu, SaveGameRow, -1});
        CallGame(c, f_002487d8, {YesSound, 0});
    }
}

// UpdateSaveSlots: the save file's read waited for only as long as it takes
extern "C" void WocUpdateSaveSlots(Ps2Cpu* c)
{
    f_00253880(c);
    if (Read32(LoadAttempted) != 0 && Read32(LoadCompleted) != 0 && Read32(LoadMessageDelay) > 0)
    {
        Write32(LoadMessageDelay, 0);
    }
}
