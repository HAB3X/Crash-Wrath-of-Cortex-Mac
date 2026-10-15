// The game's settings in the runtime's PC menus, and its language at start-up.
//
// Gameplay: "Skip cutscenes" (src/skip.cpp), "Faster loading" (src/loading.cpp) and "Language". Display: "Distance blur"
// (src/blur.cpp). The game's language follows the Mac's preferred languages as the console's would follow its own setting (the
// first of them the game has: English, French, German, Italian, Spanish, Dutch; English otherwise), or the one chosen here.
// It's set as the game's own options menu sets it, with NewLanguage (0x239CE0: the language's number into the Game's settings
// and the menus' texts copied for it): at start-up right after DefaultGame (0x1EED18, which sets English) and, when it's chosen
// in the PC menus, at the next frame. A loaded save still brings its own language, as on the PS2; the game's own menu still
// changes it.
//
// Sound: "Music volume" and "Sound effects volume", the game's own (its sound options' MUSIC VOLUME and FX VOLUME, 0-100%,
// kept in the save): the Game's settings at 0x59D518 (the name, then vibration +9, surround +0xA, the effects' volume +0xB, the
// music's +0xC, the screen's place +0xD and +0xE, the language +0xF). The game applies both every frame (UpdateGameSfx,
// 0x248BD0), so they're written there and heard at once. A loaded save brings its own, as on the PS2.
#include "woc.h"

#include "ps2/runtime.h"

#include <SDL3/SDL.h>

#include <cstring>
#include <vector>

extern "C" void f_001eed18(Ps2Cpu* c); // DefaultGame
extern "C" void f_00239ce0(Ps2Cpu* c); // NewLanguage(language)
extern "C" void f_00248750(Ps2Cpu* c); // the sound effects' frame (every frame of every loop)

int WocSkipEnabled();
int WocDistanceBlur();
void WocSetDistanceBlur(int on);
void WocSetSkipEnabled(int on);
const Ps2GameAction* WocLevelActions();
int WocLevelActionCount();
const Ps2GameAction* WocSavingActions();
int WocSavingActionCount();
void WocSavingFrame(Ps2Cpu* c);
void WocLevelsFrame();
int WocFasterLoading();
void WocSetFasterLoading(int on);
void WocLoadingFrame(Ps2Cpu* c);

using namespace Woc;

namespace
{
constexpr u32 Game = 0x59D518;
constexpr u32 EffectsVolume = Game + 0xB;
constexpr u32 MusicVolume = Game + 0xC;
constexpr u32 Language = Game + 0xF;
constexpr int Languages = 6;

// The PC menus' language choice: 0 the Mac's, then the game's six in its order
int g_language = 0;
// A language to set at the next frame (-1 none)
int g_pendingLanguage = -1;

// The Mac's preferred languages: the first the game has, English otherwise
int MacLanguage()
{
    static const char* const codes[Languages] = {"en", "fr", "de", "it", "es", "nl"};
    int count = 0;
    SDL_Locale** locales = SDL_GetPreferredLocales(&count);
    int language = 0;
    bool found = false;
    for (int i = 0; locales != nullptr && i < count && !found; i++)
    {
        for (int l = 0; l < Languages; l++)
        {
            if (locales[i]->language != nullptr && std::strcmp(locales[i]->language, codes[l]) == 0)
            {
                language = l;
                found = true;
                break;
            }
        }
    }
    SDL_free(locales);
    return language;
}

int ChosenLanguage() { return g_language == 0 ? MacLanguage() : g_language - 1; }

const char* const OffOn[] = {"Off", "On", nullptr};
const char* const LanguageChoices[] = {"Automatic", "English",  "Français",   "Deutsch", "Italiano",
                                       "Español",   "Nederlands", nullptr};
const char* const VolumeChoices[] = {"0%", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%", nullptr};

int GetLanguage()
{
    int now = static_cast<u8>(Read8(Language));
    if (g_language == 0 && now == MacLanguage())
    {
        return 0;
    }
    return now < Languages ? now + 1 : 0;
}

void SetLanguage(int choice)
{
    g_language = choice;
    g_pendingLanguage = ChosenLanguage();
}

// The volumes chosen in the PC menus (-1 none yet): written at once, and again over DefaultGame's when they're chosen before it
int g_musicVolume = -1;
int g_effectsVolume = -1;

int Volume(u32 address) { return (static_cast<u8>(Read8(address)) + 5) / 10; }

int GetMusicVolume() { return Volume(MusicVolume); }
int GetEffectsVolume() { return Volume(EffectsVolume); }

void SetMusicVolume(int choice)
{
    g_musicVolume = choice * 10;
    Write8(MusicVolume, static_cast<u8>(g_musicVolume));
}

void SetEffectsVolume(int choice)
{
    g_effectsVolume = choice * 10;
    Write8(EffectsVolume, static_cast<u8>(g_effectsVolume));
}

const Ps2GameSetting Settings[] = {
    {"Gameplay", "Skip cutscenes", OffOn, WocSkipEnabled, WocSetSkipEnabled},
    {"Gameplay", "Faster loading", OffOn, WocFasterLoading, WocSetFasterLoading},
    {"Gameplay", "Language", LanguageChoices, GetLanguage, SetLanguage},
    {"Display", "Distance blur", OffOn, WocDistanceBlur, WocSetDistanceBlur},
    {"Sound", "Music volume", VolumeChoices, GetMusicVolume, SetMusicVolume},
    {"Sound", "Sound effects volume", VolumeChoices, GetEffectsVolume, SetEffectsVolume},
};
}

const Ps2GameSetting* WocSettings() { return Settings; }
int WocSettingCount() { return sizeof(Settings) / sizeof(Settings[0]); }

// The actions: the pause menu's (src/saving.cpp), then the level select (src/levels.cpp)
static std::vector<Ps2GameAction> AllActions()
{
    std::vector<Ps2GameAction> all(WocSavingActions(), WocSavingActions() + WocSavingActionCount());
    all.insert(all.end(), WocLevelActions(), WocLevelActions() + WocLevelActionCount());
    return all;
}

const Ps2GameAction* WocActions()
{
    static const std::vector<Ps2GameAction> all = AllActions();
    return all.data();
}

int WocActionCount() { return WocSavingActionCount() + WocLevelActionCount(); }

// DefaultGame: the settings' defaults at start-up, then the volumes and the language chosen in the PC menus
extern "C" void WocDefaultGame(Ps2Cpu* c)
{
    f_001eed18(c);
    if (g_musicVolume >= 0)
    {
        Write8(MusicVolume, static_cast<u8>(g_musicVolume));
    }
    if (g_effectsVolume >= 0)
    {
        Write8(EffectsVolume, static_cast<u8>(g_effectsVolume));
    }
    int language = ChosenLanguage();
    g_pendingLanguage = -1;
    Ps2::Log("language: %d", language);
    CallGame(c, f_00239ce0, {language});
}

// Every frame: the loading screen's faster end (src/loading.cpp), a language chosen in the PC menus set, the save screen opened
// when it's asked for (src/saving.cpp), a level chosen in the pause menu started (src/levels.cpp)
extern "C" void WocFrame(Ps2Cpu* c)
{
    WocLoadingFrame(c);
    if (g_pendingLanguage >= 0)
    {
        int language = g_pendingLanguage;
        g_pendingLanguage = -1;
        CallGame(c, f_00239ce0, {language});
    }
    WocSavingFrame(c);
    WocLevelsFrame();
    f_00248750(c);
}
