// NUSOUND.IRX, the NU2 engine's sound driver on the IOP, as far as the game's boot needs it: its RPC server (0x12345) takes
// the EE's command lists (a count, then each command's number, its length in halfwords and its data) and answers with the
// driver's status block (128 words; nusound.c's SOUND_CopyIOPBuffer reads it). No sound is made yet: commands are taken and
// the statuses say the work is done.
#include "ps2/iop.h"

#include <chrono>
#include <cstdlib>

namespace
{
constexpr u32 SoundRpcId = 0x12345;

enum Status
{
    StreamCdAccess = 0,
    StreamStatus = 1,
    SfxTransferEnd = 22,
    CdStatus = 23,
    RequestLoad = 24,
    SfxTransferStatus = 26,
    ShutoffStatus = 27,
    StereoReady = 28,
};

s32 g_status[128];
bool g_trace = getenv("WOC_SOUND_TRACE") != nullptr;

// The SPU's 48 voices as the driver reports them (its key status: 1 while a voice plays). Nothing is heard: a sound effect
// "plays" for a short while, a stream until it's stopped (or a minute), so the game's waits on them end as they would
using Clock = std::chrono::steady_clock;
Clock::time_point g_voiceEnd[48];
bool g_voiceOn[48];
int g_streamVoice[8] = {-1, -1, -1, -1, -1, -1, -1, -1};

void VoiceOn(u32 voice, double seconds)
{
    if (voice < 48)
    {
        g_voiceOn[voice] = true;
        g_voiceEnd[voice] = Clock::now() + std::chrono::milliseconds(static_cast<long long>(seconds * 1000));
    }
}

void VoiceOff(u32 voice)
{
    if (voice < 48)
    {
        g_voiceOn[voice] = false;
    }
}

void KeyStatus()
{
    auto now = Clock::now();
    for (u32 word = 0; word < 3; word++)
    {
        u32 bits = 0;
        for (u32 j = 0; j < 16; j++)
        {
            u32 v = word * 16 + j;
            if (g_voiceOn[v] && now >= g_voiceEnd[v])
            {
                g_voiceOn[v] = false;
            }
            if (g_voiceOn[v])
            {
                bits |= 1u << (30 - j * 2);
            }
        }
        g_status[29 + word] = static_cast<s32>(bits);
    }
}
// "All audio off" (0x21) shows in the status for a few replies, then clears, as the driver's does when its channels have
// stopped (NuSoundKillAllAudio waits for it to rise, then to fall)
int g_shutoff = 0;

void Serve(Ps2::RpcCall& c)
{
    bool shutoffAsked = false;
    u32 count = c.In16(0);
    u32 at = 2;
    for (u32 n = 0; n < count && at + 4 <= c.inSize; n++)
    {
        u32 command = c.In16(at);
        u32 length = c.In16(at + 2);
        auto arg = [&](u32 k) -> u32 { return c.In16(at + 4 + k * 2); };
        if (g_trace && command != 9)
        {
            Ps2::Log("nusound: command %x (%u): %x %x %x %x %x %x %x", command, length, arg(0), arg(1), arg(2), arg(3), arg(4),
                     arg(5), arg(6));
        }
        switch (command)
        {
        case 0x00: // play a sound effect: sample, voice, volumes, pitch, flags
            VoiceOn(arg(1) & 0xFF, 0.5);
            break;
        case 0x01: // play a stream: stream, voice (+ bank << 8), volumes, pitch, start, envelope
        {
            u32 stream = arg(0);
            u32 voice = arg(1) & 0xFF;
            VoiceOn(voice, 60.0);
            if (stream < 8)
            {
                g_streamVoice[stream] = static_cast<int>(voice);
            }
            break;
        }
        case 0x04: // stop a voice
            VoiceOff(arg(0));
            break;
        case 0x05: // stop a stream
        {
            u32 stream = arg(0);
            if (stream < 8 && g_streamVoice[stream] >= 0)
            {
                VoiceOff(static_cast<u32>(g_streamVoice[stream]));
                g_streamVoice[stream] = -1;
            }
            break;
        }
        case 0x06: // transfer a sample to the SPU: none left
            g_status[SfxTransferStatus] = 0;
            g_status[SfxTransferEnd] = 1;
            break;
        case 0x21: // all audio off
            shutoffAsked = true;
            break;
        default:
            break;
        }
        at += 4 + length * 2;
    }
    if (shutoffAsked)
    {
        g_shutoff = 3;
    }
    if (shutoffAsked)
    {
        for (u32 v = 0; v < 48; v++)
        {
            VoiceOff(v);
        }
    }
    KeyStatus();
    g_status[ShutoffStatus] = g_shutoff > 0 ? 1 : 0;
    if (!shutoffAsked && g_shutoff > 0)
    {
        g_shutoff--;
    }
    c.out.assign(c.recvSize, 0);
    std::memcpy(c.out.data(), g_status, std::min<size_t>(c.out.size(), sizeof(g_status)));
}
}

void WocRegisterIopServers()
{
    Ps2::IopRegisterServer(SoundRpcId, "nusound", Serve);
}
