// Cutscenes skipped by holding any button, as the Crash Twinsanity native port does it: while a scene that can be skipped plays,
// the PC menus' prompt says "Hold any button to skip"; any button, key or mouse button held half a second skips it. What was
// already held as the scene began (running or jumping into it) counts only once it's let go and pressed again, and after a skip
// whatever is still held is ignored until it's let go, so the next scene isn't skipped too. The "Skip cutscenes" setting turns
// it off (the game's own skips, below, still work then).
//
// Each skip is the game's own way of ending that scene, done where the game would do it:
//   the logos and the title's opening (the cut movie 0's first world: the publishers' logos, then the flight through space to
//       the title): the first time it plays from its start; every later time (the game back at the title) StartCutMovie
//       (0x254E38) starts it TITLESTARTTIME x TITLEFPS (0x641804 x 0x641808: 6 s at 30 frames a second) before the end of its
//       first world. Held, the opening jumps to that place, as a later visit starts
//   the cut movies (the intro, the endings: cutmovie 1-4, PlayCutMovie at 0x254FB8): start pressed ends them (next_cut_movie
//       -1, the loop left); held, the same: next_cut_movie -1 and cut_on 0, the end CutSceneEndFunction (0x257580) gives them
//   the cutscenes inside a level (menu 0x12 over the play, UpdateCutMovie called from the main loop): start or cut_on 0 ends
//       them (the menu's case at 0x1E4468: the cursor's level started, or the menu closed); held, cut_on 0
//   the scenes played with the game's characters (GameMode 1: the Warp Room's first scene, and Level 0x28's, the scenes in
//       space before a warp room's first level; UpdateGameCut at 0x255680): start pressed while no fade is on ends them (0x25583C): in Level 0x28
//       the level after it starts (new_level = gamecut_newlevel), elsewhere play goes on (new_mode 0); held, the same
//
// The game's state (USA v1.01): cutmovie 0x641844 (gp-0x1EAC; -1 none, 0 the title), next_cut_movie 0x641924, cutworldix
// 0x641920, cut_on 0x63F6F8, the cut scenes 0x70E2C0 (their frame count, a float, at +8) and their instances 0x70E340 (the frame
// played at +0x70), the menu cursor 0x5A4380 (its menu at +0x5E), GameMode 0x63FD0C, Level 0x63F7B0, new_level 0x63F7B4,
// new_mode 0x63F96C, gamecut_finished 0x641854, gamecut_newlevel 0x641858, fadeval 0x63E68C.
#include "woc.h"

#include "ps2/runtime.h"

#include <atomic>

extern "C" void f_00254bd8(Ps2Cpu* c); // UpdateCutMovie
extern "C" void f_00255680(Ps2Cpu* c); // UpdateGameCut

void WocTitleFrame(Ps2Cpu* c);

using namespace Woc;

namespace
{
constexpr u32 CutMovie = 0x641844;
constexpr u32 NextCutMovie = 0x641924;
constexpr u32 CutWorld = 0x641920;
constexpr u32 CutOn = 0x63F6F8;
constexpr u32 CutScenes = 0x70E2C0;
constexpr u32 CutInstances = 0x70E340;
constexpr u32 TitleStartTime = 0x641804;
constexpr u32 TitleFps = 0x641808;
constexpr u32 MenuNumber = 0x5A4380 + 0x5E;
constexpr s8 LevelCutMenu = 0x12;
constexpr u32 GameMode = 0x63FD0C;
constexpr u32 Level = 0x63F7B0;
constexpr u32 NewLevel = 0x63F7B4;
constexpr u32 NewMode = 0x63F96C;
constexpr u32 GameCutFinished = 0x641854;
constexpr u32 GameCutNewLevel = 0x641858;
constexpr u32 FadeValue = 0x63E68C;
constexpr s32 SpaceLevel = 0x28;

// Half a second held skips; a scene is the polls in a row, a quarter of a second without one ends it (vertical blanks)
constexpr u64 HoldVblanks = 30;
constexpr u64 SceneGapVblanks = 15;

bool g_enabled = true;
std::atomic<u64> g_vblanks{0};

struct Hold
{
    bool scene = false;
    u64 lastPoll = 0;
    // Held as the scene began, or still held after a skip: ignored until it's let go
    bool ignoring = false;
    bool holding = false;
    u64 since = 0;
};
Hold g_hold;
bool g_prompt = false;

void ShowPrompt(bool show)
{
    if (show != g_prompt)
    {
        g_prompt = show;
        ps2_show_prompt(show ? "Hold any button to skip" : nullptr);
    }
}

// A frame of a scene that can be skipped: whether the hold has just completed
bool PollHold()
{
    u64 now = g_vblanks.load();
    bool held = ps2_any_input_held() != 0 && ps2_menus_open() == 0;
    if (!g_hold.scene || now - g_hold.lastPoll > SceneGapVblanks)
    {
        g_hold = {};
        g_hold.scene = true;
        g_hold.ignoring = held;
    }
    g_hold.lastPoll = now;
    ShowPrompt(true);
    if (!held)
    {
        g_hold.ignoring = false;
        g_hold.holding = false;
        return false;
    }
    if (g_hold.ignoring)
    {
        return false;
    }
    if (!g_hold.holding)
    {
        g_hold.holding = true;
        g_hold.since = now;
    }
    if (now - g_hold.since < HoldVblanks)
    {
        return false;
    }
    g_hold.holding = false;
    g_hold.ignoring = true;
    return true;
}

// The title's opening: its first world, before the place later visits start from (that place, or -1 when it's past)
float TitleSkipFrame()
{
    if (Read32(CutWorld) != 0)
    {
        return -1;
    }
    u32 scene = static_cast<u32>(Read32(CutScenes));
    u32 instance = static_cast<u32>(Read32(CutInstances));
    if (scene == 0 || instance == 0)
    {
        return -1;
    }
    float start = ReadFloat(scene + 8) - ReadFloat(TitleStartTime) * ReadFloat(TitleFps);
    return ReadFloat(instance + 0x70) < start - 1 ? start : -1;
}
}

// UpdateCutMovie: every frame of a cut movie (the title, the intro, the endings) and of a level's cutscene
extern "C" void WocUpdateCutMovie(Ps2Cpu* c)
{
    f_00254bd8(c);
    if (Read32(CutMovie) == 0)
    {
        WocTitleFrame(c);
    }
    if (!g_enabled || Read32(CutOn) == 0)
    {
        return;
    }
    s32 movie = Read32(CutMovie);
    if (movie > 0)
    {
        if (PollHold())
        {
            Ps2::Log("skip: cut movie %d", movie);
            Write32(NextCutMovie, -1);
            Write32(CutOn, 0);
        }
    }
    else if (movie == 0)
    {
        float frame = TitleSkipFrame();
        if (frame >= 0 && PollHold())
        {
            Ps2::Log("skip: the title's opening, to frame %.0f", frame);
            WriteFloat(static_cast<u32>(Read32(CutInstances)) + 0x70, frame);
        }
    }
    else if (Read8(MenuNumber) == LevelCutMenu)
    {
        if (PollHold())
        {
            Ps2::Log("skip: the level's cutscene (level %d)", Read32(Level));
            Write32(CutOn, 0);
        }
    }
}

// UpdateGameCut: every frame of play; GameMode 1 while a scene with the game's characters plays
extern "C" void WocUpdateGameCut(Ps2Cpu* c)
{
    f_00255680(c);
    if (!g_enabled || Read32(GameMode) != 1 || Read32(GameCutFinished) != 0 || Read32(FadeValue) != 0 ||
        Read32(NewLevel) != -1 || Read32(NewMode) != -1)
    {
        return;
    }
    bool space = Read32(Level) == SpaceLevel;
    if (space && Read32(GameCutNewLevel) == -1)
    {
        return;
    }
    if (PollHold())
    {
        Ps2::Log("skip: the scene in level %d", Read32(Level));
        if (space)
        {
            Write32(NewLevel, Read32(GameCutNewLevel));
        }
        else
        {
            Write32(NewMode, 0);
        }
    }
}

// At every vertical blank: the prompt taken down once no scene has polled for a while
void WocSkipVblank()
{
    u64 now = ++g_vblanks;
    if (g_prompt && now - g_hold.lastPoll > SceneGapVblanks)
    {
        g_hold.scene = false;
        ShowPrompt(false);
    }
}

int WocSkipEnabled() { return g_enabled ? 1 : 0; }

void WocSetSkipEnabled(int on)
{
    g_enabled = on != 0;
    if (!g_enabled)
    {
        g_hold.scene = false;
        ShowPrompt(false);
    }
}
