// The name's entry typed from the keyboard: while the menu is the name's (menu 3), letters go straight into the name the way
// the letter grid puts them (the game's own rules: up to 8 characters, unused ones '_', the cursor moved to DONE after the
// 8th), Backspace takes the last one off, and Return chooses DONE.
//
// The game's state (USA v1.01): the menu cursor at 0x5A4380 (its menu at +0x5E, its column and row at +0x58 and +0x59; row 4
// is DONE), the name at 0x59D518 and how many characters it has at 0x63F828 (gp - 0x3EC8).
#include "ps2/iop.h"
#include "ps2/runtime.h"

#include <cctype>

namespace
{
constexpr u32 Cursor = 0x5A4380;
constexpr u32 NameMenu = 3;
constexpr u32 Name = 0x59D518;
constexpr u32 NameLength = 0x63F828;
constexpr u32 MaxName = 8;
constexpr u8 RowDone = 4;

u8* At(u32 address) { return Ps2::EeMemory(address); }

bool OnNameEntry() { return *At(Cursor + 0x5E) == NameMenu; }

s32 Length()
{
    s32 n;
    std::memcpy(&n, At(NameLength), 4);
    return n;
}

void SetLength(s32 n) { std::memcpy(At(NameLength), &n, 4); }

void Type(char c)
{
    s32 n = Length();
    if (n < 0 || n > static_cast<s32>(MaxName))
    {
        return;
    }
    if (n >= 7)
    {
        // As the grid does: the last character, then the cursor on DONE
        At(Cursor + 0x58)[0] = 0;
        At(Cursor + 0x59)[0] = RowDone;
    }
    At(Name)[std::min<s32>(n, 7)] = static_cast<u8>(c);
    if (n < static_cast<s32>(MaxName))
    {
        SetLength(n + 1);
    }
}

void Erase()
{
    s32 n = Length();
    if (n > 0 && n <= static_cast<s32>(MaxName))
    {
        SetLength(n - 1);
        At(Name)[n - 1] = '_';
    }
}
}

int WocTyping() { return OnNameEntry() ? 1 : 0; }

void WocMenusVblank();
void WocSkipVblank();
void WocMenuTestVblank();

void WocVblank()
{
    WocMenusVblank();
    WocSkipVblank();
    WocMenuTestVblank();
    char typed[64];
    int n = ps2_take_typed(typed, sizeof(typed));
    if (n == 0 || !OnNameEntry())
    {
        return;
    }
    for (int i = 0; i < n; i++)
    {
        char c = typed[i];
        if (c == '\b')
        {
            Erase();
        }
        else if (c == '\r')
        {
            // DONE chosen: the cursor put on it and cross pressed
            At(Cursor + 0x58)[0] = 0;
            At(Cursor + 0x59)[0] = RowDone;
            ps2_press_buttons(1u << 14, 0.1);
        }
        else if (std::isalpha(static_cast<unsigned char>(c)))
        {
            Type(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        }
        else if (c == ' ' || c == '_')
        {
            Type('_');
        }
    }
}
