// Testing the game's settings and actions before the PC menus can be opened by hand: $WOC_MENU_TEST lists what to choose and
// when (game time, seconds), separated by ';':
//   S:set:NAME=CHOICE   the setting NAME given its choice number CHOICE (as the menus would: its set())
//   S:get:NAME          the setting's choice logged
//   S:run:NAME          the action NAME chosen, at S or as soon after as it's available
//   S:poke:ADDRESS=WORD the game's memory changed (hexadecimal address, decimal word): to reach a scene for a test
// e.g. WOC_MENU_TEST="12:run:Arctic Antics;30:set:Music volume=3". Each step is logged ("menu test:").
#include "ps2/iop.h"
#include "ps2/runtime.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

const Ps2GameSetting* WocSettings();
int WocSettingCount();
const Ps2GameAction* WocActions();
int WocActionCount();

namespace
{
struct Step
{
    double at;
    std::string what;
    std::string name;
    int choice;
    bool done;
};

std::vector<Step> g_steps;
bool g_read = false;
unsigned long long g_vblanks = 0;

void Read()
{
    g_read = true;
    const char* text = std::getenv("WOC_MENU_TEST");
    std::string all = text != nullptr ? text : "";
    size_t start = 0;
    while (start < all.size())
    {
        size_t end = all.find(';', start);
        std::string item = all.substr(start, end == std::string::npos ? std::string::npos : end - start);
        start = end == std::string::npos ? all.size() : end + 1;
        size_t c1 = item.find(':');
        size_t c2 = c1 == std::string::npos ? c1 : item.find(':', c1 + 1);
        if (c2 == std::string::npos)
        {
            continue;
        }
        Step step{std::atof(item.c_str()), item.substr(c1 + 1, c2 - c1 - 1), item.substr(c2 + 1), 0, false};
        size_t equals = step.name.find('=');
        if (equals != std::string::npos)
        {
            step.choice = static_cast<int>(std::strtol(step.name.c_str() + equals + 1, nullptr, 0));
            step.name.resize(equals);
        }
        g_steps.push_back(step);
    }
}

const Ps2GameSetting* Setting(const std::string& name)
{
    for (int i = 0; i < WocSettingCount(); i++)
    {
        if (name == WocSettings()[i].name)
        {
            return &WocSettings()[i];
        }
    }
    return nullptr;
}

const Ps2GameAction* Action(const std::string& name)
{
    for (int i = 0; i < WocActionCount(); i++)
    {
        if (name == WocActions()[i].name)
        {
            return &WocActions()[i];
        }
    }
    return nullptr;
}
}

void WocMenuTestVblank()
{
    if (!g_read)
    {
        Read();
    }
    double now = static_cast<double>(++g_vblanks) / 60.0;
    for (Step& step : g_steps)
    {
        if (step.done || now < step.at)
        {
            continue;
        }
        if (step.what == "set" || step.what == "get")
        {
            step.done = true;
            const Ps2GameSetting* s = Setting(step.name);
            if (s == nullptr)
            {
                Ps2::Log("menu test: no setting %s", step.name.c_str());
                continue;
            }
            if (step.what == "set")
            {
                s->set(step.choice);
            }
            Ps2::Log("menu test: %.1f s: %s is %s", now, s->name, s->choices[s->get()]);
        }
        else if (step.what == "poke")
        {
            step.done = true;
            u32 address = static_cast<u32>(std::strtoul(step.name.c_str(), nullptr, 16));
            s32 word = step.choice;
            std::memcpy(Ps2::EeMemory(address), &word, 4);
            Ps2::Log("menu test: %.1f s: %08x = %d", now, address, word);
        }
        else if (step.what == "run")
        {
            const Ps2GameAction* a = Action(step.name);
            if (a == nullptr)
            {
                step.done = true;
                Ps2::Log("menu test: no action %s", step.name.c_str());
                continue;
            }
            if (a->available == nullptr || a->available() != 0)
            {
                step.done = true;
                Ps2::Log("menu test: %.1f s: %s chosen", now, a->name);
                a->run();
            }
        }
    }
}
