// The level select (the PC menus' Extras, a group for each warp room): any of the game's 35 levels started, from play or from
// the title's main menu.
//
// In play it's done as the game's own level changes are (a warp room's portal, HubLevelSelect; the pause menu's quit): new_level
// set, after which the main loop (0x1C7F88) fades out, leaves the level and loads the new one (the warp room's position, the
// scene in space before a new warp room's first level, GetSpaceCut at 0x255CD0, all as after a portal). Nothing is
// unlocked or written to the save: the level is played in the game as it is, and finishing it goes back to the Warp Room as from
// any level.
//
// At the title it's the main menu's NEW GAME without the name's entry and the intro: the title's main loop is left the way the
// name's DONE leaves it (the menu closed with NewMenu, a fade to black), with the game's new Game (NewGame ran as the title
// started) named CRASH and Level (which the title has set to the Warp Room, 0x25) the chosen level's, so the main loop loads it
// instead. Done from UpdateCutMovie, which the title's loop calls every frame (src/skip.cpp), where NewMenu can be called.
//
// The levels, as the warp rooms list them (HData at 0x598A50: six rows of six levels; the sixth row is the secret levels, its
// last entry not a level), and their names (LevelName, the text table's rows from 0x5E0680).
//
// The game's state (USA v1.01): Level 0x63F7B0, new_level 0x63F7B4, new_mode 0x63F96C, GameMode 0x63FD0C, Paused 0x63F70C, Demo
// 0x63F970, warp_level 0x63F7BC (a portal's level while Crash warps into it, HubLevelSelect at 0x1DBC30; -1 none), cutmovie
// 0x641844 (-1 in play, 0 the title), next_cut_movie 0x641924, cutworldix 0x641920, fadeval 0x63E68C, fade_rate 0x63E694, the
// menu cursor 0x5A4380 (its menu at +0x5E: -1 none, 0 the title's main menu), the name 0x59D518, NewMenu 0x1E2910.
#include "woc.h"

#include "ps2/runtime.h"

extern "C" void f_001e2910(Ps2Cpu* c); // NewMenu(cursor, menu, row, level)

using namespace Woc;

namespace
{
constexpr u32 Level = 0x63F7B0;
constexpr u32 NewLevel = 0x63F7B4;
constexpr u32 NewMode = 0x63F96C;
constexpr u32 GameMode = 0x63FD0C;
constexpr u32 Paused = 0x63F70C;
constexpr u32 WarpLevel = 0x63F7BC;
constexpr u32 Demo = 0x63F970;
constexpr u32 CutMovie = 0x641844;
constexpr u32 NextCutMovie = 0x641924;
constexpr u32 CutWorld = 0x641920;
constexpr u32 FadeValue = 0x63E68C;
constexpr u32 FadeRate = 0x63E694;
constexpr u32 Cursor = 0x5A4380;
constexpr u32 Name = 0x59D518;
constexpr s32 Title = 0x23;
constexpr s32 WarpRoom = 0x25;

struct LevelInfo
{
    const char* room;
    const char* name;
    s32 level;
};

constexpr LevelInfo Levels[] = {
    {"Warp Room 1", "Arctic Antics", 0x07},
    {"Warp Room 1", "Tornado Alley", 0x0D},
    {"Warp Room 1", "Bamboozled", 0x0B},
    {"Warp Room 1", "Wizards and Lizards", 0x11},
    {"Warp Room 1", "Compactor Reactor", 0x04},
    {"Warp Room 1", "Rumble in the Roks", 0x15},
    {"Warp Room 2", "Jungle Rumble", 0x08},
    {"Warp Room 2", "Sea Shell Shenanigans", 0x22},
    {"Warp Room 2", "Banzai Bonsai", 0x0C},
    {"Warp Room 2", "That Sinking Feeling", 0x12},
    {"Warp Room 2", "H2 Oh No", 0x02},
    {"Warp Room 2", "Drain Damage", 0x17},
    {"Warp Room 3", "The Gauntlet", 0x0A},
    {"Warp Room 3", "Tsunami", 0x0E},
    {"Warp Room 3", "Smokey and the Bandicoot", 0x03},
    {"Warp Room 3", "Eskimo Roll", 0x00},
    {"Warp Room 3", "Fahrenheit Frenzy", 0x0F},
    {"Warp Room 3", "Crashes to Ashes", 0x16},
    {"Warp Room 4", "Avalanche", 0x05},
    {"Warp Room 4", "Droid Void", 0x14},
    {"Warp Room 4", "Crashteroids", 0x1A},
    {"Warp Room 4", "Coral Canyon", 0x06},
    {"Warp Room 4", "Weathering Heights", 0x1E},
    {"Warp Room 4", "Atmospheric Pressure", 0x18},
    {"Warp Room 5", "Crash and Burn", 0x01},
    {"Warp Room 5", "Gold Rush", 0x13},
    {"Warp Room 5", "Medieval Madness", 0x10},
    {"Warp Room 5", "Crate Balls of Fire", 0x1F},
    {"Warp Room 5", "Cortex Vortex", 0x21},
    {"Warp Room 5", "Crunch Time", 0x19},
    {"Secret Levels", "Knight Time", 0x1B},
    {"Secret Levels", "Ghost Town", 0x1C},
    {"Secret Levels", "Ice Station Bandicoot", 0x1D},
    {"Secret Levels", "Solar Bowler", 0x20},
    {"Secret Levels", "Force of Nature", 0x09},
};
constexpr int LevelCount = sizeof(Levels) / sizeof(Levels[0]);

// The level chosen at the title, started by the title's next frame (-1 none)
s32 g_titleLevel = -1;

s8 Menu() { return Read8(Cursor + 0x5E); }

// In play: a level (not the title, the scenes in space, a demo), nothing leaving it, no scene of the game's own, no menu
bool InPlay()
{
    s32 level = Read32(Level);
    return Read32(CutMovie) == -1 && level >= 0 && (level < Title || level == WarpRoom) && Read32(Demo) == 0 && Read32(GameMode) == 0 &&
           Read32(NewLevel) == -1 && Read32(NewMode) == -1 && Read32(WarpLevel) == -1 && Read32(FadeValue) == 0 &&
           Read32(Paused) == 0 && Menu() == -1;
}

// The title's main menu up, nothing chosen yet
bool AtTitle()
{
    return Read32(CutMovie) == 0 && Read32(CutWorld) == 1 && Menu() == 0 && Read32(NextCutMovie) == -1 &&
           Read32(FadeValue) == 0 && Read32(FadeRate) <= 0 && g_titleLevel == -1;
}

int CanStart() { return InPlay() || AtTitle() ? 1 : 0; }

void Start(int index)
{
    const LevelInfo& info = Levels[index];
    if (InPlay())
    {
        Ps2::Log("level select: %s (level %d) from level %d", info.name, info.level, Read32(Level));
        Write32(NewLevel, info.level);
    }
    else if (AtTitle())
    {
        Ps2::Log("level select: %s (level %d) from the title", info.name, info.level);
        g_titleLevel = info.level;
    }
}

template <int Index>
void StartLevel()
{
    Start(Index);
}

template <int... Index>
struct Table
{
    static constexpr Ps2GameAction actions[] = {
        {"Extras", Levels[Index].room, Levels[Index].name, CanStart, StartLevel<Index>}...};
};

template <int N, int... Index>
struct MakeTable : MakeTable<N - 1, N - 1, Index...>
{
};

template <int... Index>
struct MakeTable<0, Index...>
{
    using type = Table<Index...>;
};

using LevelTable = MakeTable<LevelCount>::type;
}

const Ps2GameAction* WocLevelActions() { return LevelTable::actions; }
int WocLevelActionCount() { return LevelCount; }

// From the title's loop (UpdateCutMovie's override): a level chosen there started as NEW GAME's DONE leaves the title
void WocTitleFrame(Ps2Cpu* c)
{
    if (g_titleLevel == -1)
    {
        return;
    }
    s32 level = g_titleLevel;
    g_titleLevel = -1;
    if (Read32(CutMovie) != 0 || Menu() != 0)
    {
        return;
    }
    std::memcpy(At(Name), "CRASH___", 9);
    Write32(Level, level);
    CallGame(c, f_001e2910, {static_cast<s32>(Cursor), -1, -1, -1});
    Write32(FadeRate, 8);
}
