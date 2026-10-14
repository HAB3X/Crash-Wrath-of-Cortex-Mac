// The menus with a mouse and a PC keyboard: pointing at a menu's line moves its cursor there, clicking it chooses it (the
// cursor put on it and cross pressed), and the runtime makes Return cross and Backspace triangle while a menu is up.
//
// Where the lines are: every menu line is drawn by one function (0x1E5478, MenuItem(cursor, text, &x, &y, &line)), which
// draws the line at (x, y) in the font's screen units when it's among the cursor's shown lines, then moves y down a line
// and counts the line. It's overridden to note each drawn line (its menu, number and place) and then run as before.
//
// The game's state (USA v1.01): the menu cursor at 0x5A4380 (its menu at +0x5E, -1 for none; the current line at +0x59, the
// first and last shown at +0x5B and +0x5D).
#include "ps2/iop.h"
#include "ps2/recomp.h"
#include "ps2/runtime.h"

#include <cmath>
#include <mutex>
#include <vector>

extern "C" void f_001e5478(Ps2Cpu* c);

namespace
{
constexpr u32 Cursor = 0x5A4380;

struct Drawn
{
    s8 menu;
    s8 line;
    float x;
    float y;
};

// The lines drawn since the last vblank, and the last complete set
std::vector<Drawn> g_drawing;
std::vector<Drawn> g_shown;
bool g_logged = false;

u8* At(u32 address) { return Ps2::EeMemory(address); }

float ReadFloat(u32 address)
{
    float v;
    std::memcpy(&v, At(address), 4);
    return v;
}

s8 Menu() { return static_cast<s8>(*At(Cursor + 0x5E)); }

// The font's screen units to the picture's 0-1: x 0 is the middle, y runs up (measured on the title's lines: -0.69 and -0.81
// are drawn at 0.83 and 0.885 of the picture's height)
float PictureX(float x) { return 0.5f + x * 0.5f; }
float PictureY(float y) { return 0.514f - y * 0.458f; }

// The line at a place in the picture: the nearest line's in its row (a line's height either side of its centre)
const Drawn* LineAt(float px, float py)
{
    const Drawn* best = nullptr;
    float bestDistance = 0.03f;
    for (const Drawn& d : g_shown)
    {
        if (d.menu != Menu())
        {
            continue;
        }
        float dy = std::fabs(PictureY(d.y) - py);
        float dx = std::fabs(PictureX(d.x) - px);
        if (dy < bestDistance && dx < 0.35f)
        {
            bestDistance = dy;
            best = &d;
        }
    }
    return best;
}
}

extern "C" void WocMenuItem(Ps2Cpu* c)
{
    u32 cursor = c->r[4].uw[0];
    u32 xAt = c->r[6].uw[0];
    u32 yAt = c->r[7].uw[0];
    u32 lineAt = c->r[8].uw[0];
    s32 line;
    std::memcpy(&line, At(lineAt), 4);
    s8 first = static_cast<s8>(*At(cursor + 0x5B));
    s8 last = static_cast<s8>(*At(cursor + 0x5D));
    if (cursor == Cursor && line >= first && line <= last)
    {
        g_drawing.push_back({static_cast<s8>(*At(cursor + 0x5E)), static_cast<s8>(line), ReadFloat(xAt), ReadFloat(yAt)});
    }
    f_001e5478(c);
}

int WocMenu() { return Menu() != -1 ? 1 : 0; }

void WocMenusVblank()
{
    if (!g_drawing.empty() || Menu() == -1)
    {
        g_shown.swap(g_drawing);
        g_drawing.clear();
    }
    if (getenv("WOC_MENU_TRACE") != nullptr && !g_shown.empty() && !g_logged)
    {
        g_logged = true;
        for (const Drawn& d : g_shown)
        {
            Ps2::Log("menu %d line %d at %.3f %.3f", d.menu, d.line, d.x, d.y);
        }
    }
    if (Menu() == -1)
    {
        g_logged = false;
        return;
    }
    float x, y;
    if (ps2_mouse_moved(&x, &y))
    {
        if (const Drawn* d = LineAt(x, y))
        {
            *At(Cursor + 0x59) = static_cast<u8>(d->line);
        }
    }
    while (ps2_take_click(&x, &y))
    {
        if (const Drawn* d = LineAt(x, y))
        {
            *At(Cursor + 0x59) = static_cast<u8>(d->line);
            ps2_press_buttons(1u << 14, 0.1);
        }
    }
}
