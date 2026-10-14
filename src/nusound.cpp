// NUSOUND.IRX, the NU2 engine's sound driver on the IOP, high level: its RPC server (0x12345) takes the EE's command lists (a
// count, then each command's number, its length in halfwords and its data) and answers with the driver's status block (128
// words; nusound.c's SOUND_CopyIOPBuffer reads it), and its stream thread runs at its hardware timer's 60 ticks a second. What it
// does follows the module's own code (SYS/NUSOUND.IRX, read from its disassembly): the samples' table the EE sends (SFXINFO),
// their files found in SFX.DAT (a NuDat archive) or on the disc, the resident sound effects read into the sound memory one after
// another from the address the EE gives, the streams (music, voices) played from a double buffer in the sound memory that the
// stream thread refills half a buffer at a time as the voice's NAX leaves each half, the EE's go-ahead for each of a stream's
// disc reads (its echo of the request in the status), stereo pairs started together, and the voices' key status from their
// envelopes. The SPU2 is the runtime's (ps2/sound.h), driven through libsd's calls as the module makes them.
//
// How it differs from the module: its disc reads (sceCdRead with a callback, a sector at a time at the ends of a read) are done
// at once where it starts them, so a stream's read never spans a timer tick and the next half is read when it's needed rather
// than ahead; the resident sound effects are read and transferred within the command that asks for one (the module does it at
// its next ticks while the EE polls the status); no CD errors happen, so the module's pause and resume of streams on one isn't
// done. The reverb presets (command 0x13) aren't done: the game never asks for them.
#include "ps2/iop.h"
#include "ps2/sound.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace
{
using namespace Ps2;

constexpr u32 SoundRpcId = 0x12345;
constexpr u32 MaxSamples = 300;
constexpr u32 MaxStreams = 48;
constexpr u32 Voices = 48;
// A VAG file's header, skipped when its data is read
constexpr u32 VagHeader = 0x30;
// The driver's hardware timer: 16,666 microseconds (0x411A), in the SPU2's samples
constexpr u32 TimerSamples = 800;

bool g_trace = getenv("WOC_SOUND_TRACE") != nullptr;

// --- the status block's words
enum Status
{
    StreamCdAccess = 0,
    StreamActive = 1,
    StreamVoice = 2,
    StreamId = 6,
    StreamNax = 10,
    StreamHalf = 14,
    StreamEnvelope = 18,
    SfxTransferEnd = 22,
    StreamBusyStatus = 23,
    RequestLoadStatus = 24,
    LoadCounterStatus = 25,
    SfxTransferStatus = 26,
    ShutoffStatus = 27,
    StereoStatus = 28,
    KeyStatusWords = 29,
};

// --- the samples: what the EE sent (SFXINFO, 0x2C bytes) and where each one's file is
struct SampleInfo
{
    char name[32];
    s32 pitch;
    s32 loopInfo;
    s32 id;
};
static_assert(sizeof(SampleInfo) == 0x2C);

struct FileInfo
{
    u32 lsn = 0;
    u32 size = 0; // 0: not found
};

SampleInfo g_samples[MaxSamples];
FileInfo g_files[MaxSamples];
u32 g_sampleCount = 0;
u32 g_globalSampleCount = 0;
bool g_anyFiles = false;

// The resident sound effects: each one's ID and its address in the sound memory, and where the next goes
struct SpotFx
{
    s32 id = -1;
    u32 address = 0;
};
SpotFx g_spotFx[MaxSamples + 1];
u32 g_spuEnd = 0;
u32 g_globalSpuEnd = 0;
s32 g_resStatus = 0;

// The device prefix and terminator of the files' names ("cdrom0:\" and ";1"), from the EE's init
std::string g_filePath;
std::string g_fileTerm;

// --- NuDat archives (SFX.DAT): at offset 0 the directory's offset and size; the directory is a word, the files' count and
// their entries (offset, size, unpacked size, flags), the names' tree's count and nodes (a child, a sibling, a name's offset;
// a child below 1 is the file numbered minus it), the names' size and the names. Paths are found from node 1, a part at a time
// ('\\' between), names compared without case
struct Dat
{
    std::string name;
    u32 lsn = 0;
    struct File
    {
        u32 offset, size;
    };
    std::vector<File> files;
    struct Node
    {
        s16 child, sibling;
        u32 name;
    };
    std::vector<Node> tree;
    std::vector<char> names;
    bool open = false;
};
Dat g_globalDat;
Dat g_localDat;
std::string g_newGlobalDat;
std::string g_newLocalDat;

bool ReadBytes(u32 lsn, u32 offset, u32 size, u8* out)
{
    u32 first = offset / 2048;
    u32 last = (offset + size + 2047) / 2048;
    std::vector<u8> sectors((last - first) * 2048);
    if (!DiscReadSectors(lsn + first, last - first, sectors.data()))
    {
        return false;
    }
    std::memcpy(out, sectors.data() + (offset - first * 2048), size);
    return true;
}

// "cdrom0:\SFX.DAT;1" as the disc's path
std::string DiscPath(const std::string& name)
{
    size_t colon = name.find(':');
    return colon == std::string::npos ? name : name.substr(colon + 1);
}

bool DatOpen(Dat& dat, const std::string& name)
{
    dat = Dat{};
    dat.name = name;
    u32 size;
    std::string path = g_filePath + name + g_fileTerm;
    if (!DiscFind(DiscPath(path), dat.lsn, size))
    {
        Log("nusound: %s not on the disc", path.c_str());
        return false;
    }
    u32 head[2];
    if (!ReadBytes(dat.lsn, 0, 8, reinterpret_cast<u8*>(head)) || head[1] < 12 || head[0] + head[1] > size)
    {
        return false;
    }
    std::vector<u8> d(head[1]);
    ReadBytes(dat.lsn, head[0], head[1], d.data());
    auto word = [&](u32 at) -> u32 {
        u32 v = 0;
        if (at + 4 <= d.size())
        {
            std::memcpy(&v, &d[at], 4);
        }
        return v;
    };
    u32 at = 4;
    u32 count = word(at);
    at += 4;
    for (u32 i = 0; i < count && at + 16 <= d.size(); i++, at += 16)
    {
        dat.files.push_back({word(at), word(at + 4)});
    }
    u32 nodes = word(at);
    at += 4;
    for (u32 i = 0; i < nodes && at + 8 <= d.size(); i++, at += 8)
    {
        u32 w = word(at);
        dat.tree.push_back({static_cast<s16>(w & 0xFFFF), static_cast<s16>(w >> 16), word(at + 4)});
    }
    at += 4;
    dat.names.assign(d.begin() + std::min<size_t>(at, d.size()), d.end());
    dat.names.push_back(0);
    dat.open = true;
    Log("nusound: %s: %zu files", path.c_str(), dat.files.size());
    return true;
}

bool SameName(const char* a, const std::string& b)
{
    size_t i = 0;
    for (; a[i] != 0 && i < b.size(); i++)
    {
        if (std::toupper(static_cast<unsigned char>(a[i])) != std::toupper(static_cast<unsigned char>(b[i])))
        {
            return false;
        }
    }
    return a[i] == 0 && i == b.size();
}

// A file's number in the archive, or -1
int DatFind(const Dat& dat, const std::string& path)
{
    if (!dat.open || dat.tree.size() < 2)
    {
        return -1;
    }
    size_t slash = path.find('\\');
    std::string part = path.substr(0, slash);
    u32 node = 1;
    for (u32 steps = 0; node != 0 && node < dat.tree.size() && steps < 100000; steps++)
    {
        const Dat::Node& n = dat.tree[node];
        const char* name = n.name < dat.names.size() ? &dat.names[n.name] : "";
        if (!SameName(name, part))
        {
            node = static_cast<u32>(static_cast<s32>(n.sibling));
            continue;
        }
        if (slash == std::string::npos)
        {
            return n.child < 1 ? -n.child : -1;
        }
        size_t start = slash + 1;
        slash = path.find('\\', start);
        part = path.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
        node = static_cast<u32>(static_cast<s32>(n.child));
    }
    return -1;
}

void SetDat(Dat& dat, const std::string& name)
{
    if (dat.open && dat.name == name)
    {
        return;
    }
    dat = Dat{};
    if (!name.empty())
    {
        DatOpen(dat, name);
    }
}

// InitADPCMFiles: the samples from first on found, in the archive (the level's own, or the global one) or on the disc
void FindFiles(u32 first)
{
    Dat* dat = &g_globalDat;
    if (first == 0)
    {
        SetDat(g_globalDat, g_newGlobalDat);
    }
    else
    {
        SetDat(g_localDat, g_newLocalDat);
        if (!g_localDat.open)
        {
            SetDat(g_globalDat, g_newGlobalDat);
        }
        else
        {
            dat = &g_localDat;
        }
    }
    u32 found = 0;
    for (u32 i = first; i < MaxSamples; i++)
    {
        const SampleInfo& s = g_samples[i];
        if (s.pitch == 0)
        {
            continue;
        }
        std::string name(s.name, strnlen(s.name, sizeof(s.name)));
        FileInfo info;
        int file = DatFind(*dat, name);
        if (file >= 0 && static_cast<u32>(file) < dat->files.size())
        {
            info.lsn = dat->lsn + dat->files[file].offset / 2048;
            info.size = dat->files[file].size;
        }
        else if (!DiscFind(DiscPath(g_filePath + name + g_fileTerm), info.lsn, info.size))
        {
            info.size = 0;
            Log("nusound: sample %s (%x) not found", name.c_str(), s.id);
        }
        g_files[i] = info;
        if (info.size != 0)
        {
            g_anyFiles = true;
            found++;
        }
    }
    Log("nusound: %u samples' files found (from %u)", found, first);
}

int FindSample(s32 id)
{
    for (u32 i = 0; i < g_sampleCount; i++)
    {
        if (g_samples[i].id == id)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// --- the voices
u8 g_keyStatus[Voices];
u16 g_pausePitch[Voices];
u32 g_reverbVoices[2];

u16 VoiceEntry(u32 voice) { return static_cast<u16>(voice < 24 ? voice << 1 : ((voice - 24) << 1) | 1); }
u32 VoiceBit(u32 voice) { return 1u << (voice < 24 ? voice : voice - 24); }
u16 CoreOf(u32 voice) { return voice < 24 ? 0 : 1; }

void SetVolume(u32 voice, u16 left, u16 right)
{
    SdSetParam(VoiceEntry(voice) | SD_VP_VOLL, left);
    SdSetParam(VoiceEntry(voice) | SD_VP_VOLR, right);
}

void SetPitch(u32 voice, u16 pitch)
{
    SdSetParam(VoiceEntry(voice) | SD_VP_PITCH, pitch);
    if (voice < Voices)
    {
        g_pausePitch[voice] = pitch;
    }
}

// Key off with the fastest release
void KeyOff(u32 voice)
{
    SdSetSwitch(CoreOf(voice) | SD_S_KOFF, VoiceBit(voice));
    SdSetParam(VoiceEntry(voice) | SD_VP_ADSR2, 0x1FC0);
}

// SetVoiceData: the volumes, the pitch, the envelope (attack from the low byte, release from the high one) and the start
void SetVoice(u32 voice, u16 pitch, u16 left, u16 right, u32 address, u32 envelope)
{
    SetVolume(voice, left, right);
    SetPitch(voice, pitch);
    SdSetParam(VoiceEntry(voice) | SD_VP_ADSR1, static_cast<u16>(((envelope & 0xFF) << 8) | 0xFF));
    SdSetParam(VoiceEntry(voice) | SD_VP_ADSR2, static_cast<u16>((envelope >> 8) | 0x1FC0));
    SdSetAddr(VoiceEntry(voice) | SD_VA_SSA, address);
}

void KeyOn(u32 voice) { SdSetSwitch(CoreOf(voice) | SD_S_KON, VoiceBit(voice)); }

// --- the streams
enum State : u8
{
    Playing = 0,
    Loaded = 1,
    Queued = 2,
    Idle = 3,
    SpotLoad = 4,
};
// How a stream's file is read: looping (0), once (1), read to its end (2), a resident sound effect (0x7F)
enum Mode : u8
{
    Looping = 0,
    OneShot = 1,
    AllRead = 2,
    Spot = 0x7F,
};
// Which half of the buffer is to be filled next (0 or 1), or 3 once the file is all in
constexpr u8 HalvesDone = 3;

struct Stream
{
    bool active = false;
    u8 state = Idle;
    u8 voice = 0;
    u8 half = 0;
    u8 mode = Looping;
    s16 left = 0;
    s16 right = 0;
    s16 pitch = 0;
    s32 id = 0;
    u32 envelope = 0;
    // What play stream asked for (USR_StartStream), started at the next tick
    u32 askVoice = 0, askLeft = 0, askRight = 0, askId = 0, askPitch = 0, askStart = 0, askEnvelope = 0;
    // The buffer in the sound memory (two halves) the EE gave it
    bool allocated = false;
    u32 base = 0;
    u32 size = 0;
    u32 halfSize = 0;
    // The file: where it is, how far it's read, what's left
    u32 lsn = 0;
    u32 fileSize = 0;
    u32 readPos = 0;
    u32 remaining = 0;
    // NAX and ENVX at the last tick
    u32 nax = 0;
    u32 envx = 0;
};
Stream g_streams[MaxStreams];
u32 g_maxStreams = 5;
s32 g_requestLoad = -1;
u32 g_loadFromEE = 0xFFFF;
u32 g_loadCounter = 0;
u32 g_direction = 0;
bool g_shutDown = false;
bool g_stereoStarted = false;
u32 g_stereo1 = 0, g_stereo2 = 0;
u16 g_stereoPitch = 0;
s32 g_status[128];

u32 Nax(u32 voice) { return SdGetAddr(VoiceEntry(voice) | SD_VA_NAX); }
u16 Envx(u32 voice) { return SdGetParam(VoiceEntry(voice) | SD_VP_ENVX); }

void StopStream(u32 which)
{
    auto stop = [](Stream& s) {
        s.active = false;
        s.state = Idle;
        SetPitch(s.voice, 0);
        KeyOff(s.voice);
        g_keyStatus[s.voice % Voices] = 0;
    };
    if (which < 0x40)
    {
        // By voice
        for (Stream& s : g_streams)
        {
            if (s.active && s.voice == which)
            {
                stop(s);
                return;
            }
        }
        return;
    }
    Stream& s = g_streams[(which & 0x3F) % MaxStreams];
    if (s.active)
    {
        stop(s);
    }
}

// USR_StartStream: a stream asked for (bank 0x80: the first free one), started at the next tick
void AskStream(u32 voiceBank, u32 left, u32 right, u32 id, u32 pitch, u32 start, u32 envelope)
{
    u32 bank = (voiceBank >> 8) & 0xFF;
    StopStream(voiceBank & 0xFF);
    s32 slot = -1;
    if (bank == 0x80)
    {
        for (u32 i = 0; i < g_maxStreams; i++)
        {
            if (!g_streams[i].active)
            {
                slot = static_cast<s32>(i);
                break;
            }
        }
    }
    else if (bank < g_maxStreams)
    {
        slot = static_cast<s32>(bank);
    }
    if (slot < 0)
    {
        if (g_trace)
        {
            Log("nusound: no stream free for %x", id);
        }
        return;
    }
    Stream& s = g_streams[slot];
    s.state = start == Spot ? SpotLoad : Queued;
    s.askVoice = voiceBank & 0xFF;
    s.askLeft = left;
    s.askRight = right;
    s.askId = id;
    s.askPitch = pitch;
    s.askStart = start;
    s.askEnvelope = envelope;
    s.active = true;
}

// StartStream: the stream's file opened and its settings taken. False when its sample has no file
bool StartStream(Stream& s)
{
    int sample = FindSample(static_cast<s32>(s.askId));
    if (sample < 0 || g_files[sample].size <= VagHeader)
    {
        Log("nusound: stream %x has no file", s.askId);
        s.active = false;
        return false;
    }
    const FileInfo& file = g_files[sample];
    s.id = static_cast<s32>(s.askId);
    s.envelope = s.askEnvelope;
    s.half = 0;
    s.left = static_cast<s16>(s.askLeft);
    s.right = static_cast<s16>(s.askRight);
    s.voice = static_cast<u8>(s.askVoice % Voices);
    s.readPos = VagHeader;
    s.mode = static_cast<u8>(s.askStart == 2 ? g_samples[sample].loopInfo : s.askStart);
    // Pitch 1 is the sample's own: a rate in Hz above 11,024 (else already the SPU2's pitch)
    s32 pitch = static_cast<s32>(s.askPitch);
    if (pitch == 1)
    {
        pitch = g_samples[sample].pitch;
        if (pitch > 0x2B10)
        {
            pitch = (pitch << 12) / 48000;
        }
    }
    s.pitch = static_cast<s16>(pitch);
    s.lsn = file.lsn;
    s.fileSize = file.size;
    s.remaining = file.size - VagHeader;
    return true;
}

// LoadDataFromDisk: the next size bytes of the file into out. A looping file goes on from its start (its end's flags cleared);
// one played once ends with its last blocks' flags set to stop the voice
void ReadChunk(Stream& s, std::vector<u8>& out, u32 size)
{
    out.assign(size, 0);
    if (s.mode == AllRead || s.fileSize <= VagHeader)
    {
        return;
    }
    auto read = [&](u32 count) {
        // Wraps to the data's start at the file's end
        u32 done = 0;
        while (done < count)
        {
            if (s.readPos >= s.fileSize)
            {
                s.readPos = VagHeader;
            }
            u32 n = std::min(count - done, s.fileSize - s.readPos);
            ReadBytes(s.lsn, s.readPos, n, out.data() + done);
            s.readPos += n;
            done += n;
        }
    };
    u32 left = s.remaining;
    if (size < left)
    {
        read(size);
        s.remaining -= size;
    }
    else if (s.mode == Looping)
    {
        read(size);
        if (left >= 0x20)
        {
            out[left - 0xF] = 0;
            out[left - 0x1F] = 0;
        }
        s.readPos = VagHeader + (size - left) % (s.fileSize - VagHeader);
        s.remaining = s.fileSize - s.readPos;
    }
    else if (s.mode == OneShot)
    {
        read(left);
        s.mode = AllRead;
        if (left >= 0x20)
        {
            out[left - 0x1F] = 1;
            out[left - 0xF] = 7;
        }
        s.remaining = 0;
    }
    else
    {
        read(left);
        s.remaining = 0;
    }
}

// UpdateStream: a chunk read and written to the buffer: all of it (part 2) or a half. The blocks' flags make the voice go
// round the buffer: the first block starts the loop, the last of the first half goes on, the last of the second goes back
void FillBuffer(Stream& s, u32 part)
{
    std::vector<u8> chunk;
    u32 size = part == 2 ? s.size : s.halfSize;
    ReadChunk(s, chunk, size);
    u32 address = s.base + (part == 1 ? s.halfSize : 0);
    if (s.mode == AllRead)
    {
        SdVoiceTransWrite(address, chunk.data(), size);
        s.half = HalvesDone;
        return;
    }
    if (size >= 0x20 && s.mode != Spot)
    {
        chunk[1] = part == 1 ? 2 : 6;
        chunk[0x11] = 2;
        chunk[size - 0xF] = part == 0 ? 2 : 3;
    }
    SdVoiceTransWrite(address, chunk.data(), size);
    if (part != 2)
    {
        s.half = part == 0 ? 1 : 0;
    }
}

// HandleEERequest: the work the EE let the stream do (its echo of the request)
void ServeRequest(u32 index)
{
    Stream& s = g_streams[index];
    if (!s.active)
    {
        return;
    }
    switch (s.state)
    {
    case Queued:
        FillBuffer(s, 2);
        s.state = Loaded;
        break;
    case Loaded:
        SetVoice(s.voice, static_cast<u16>(s.pitch), static_cast<u16>(s.left), static_cast<u16>(s.right), s.base, s.envelope);
        KeyOn(s.voice);
        g_keyStatus[s.voice] = 1;
        s.state = Playing;
        if (g_trace)
        {
            Log("nusound: stream %u (%x) plays on voice %u, pitch %x", index, s.id, s.voice, static_cast<u16>(s.pitch));
        }
        break;
    case Playing:
        if (s.half == 0 || s.half == 1)
        {
            FillBuffer(s, s.half);
        }
        break;
    default:
        break;
    }
}

// CheckTwoStreamsStatus: a stereo pair's voices given their pitch together once both play
void CheckStereo()
{
    if (!g_stereoStarted)
    {
        return;
    }
    bool first = false, second = false;
    for (const Stream& s : g_streams)
    {
        if (s.active && s.state == Playing)
        {
            first |= s.voice == g_stereo1;
            second |= s.voice == g_stereo2;
        }
    }
    if (first && second)
    {
        SetPitch(g_stereo1, g_stereoPitch);
        SetPitch(g_stereo2, g_stereoPitch);
        g_stereoStarted = false;
    }
}

// SOUND_DoShutOff: every stream and voice stopped
void ShutOff()
{
    g_loadFromEE = 0xFFFF;
    g_requestLoad = -1;
    for (u32 v = 0; v < Voices; v++)
    {
        if (v < MaxStreams)
        {
            g_streams[v].active = false;
            g_streams[v].state = Idle;
            g_streams[v].envx = 0;
        }
        SetPitch(v, 0);
        SdSetParam(VoiceEntry(v) | SD_VP_ADSR2, 0x1FC0);
        SdSetSwitch(CoreOf(v) | SD_S_KOFF, VoiceBit(v));
        g_keyStatus[v] = 0;
    }
    g_resStatus = 0;
    g_stereoStarted = false;
    g_stereo1 = g_stereo2 = 0;
    g_stereoPitch = 0;
    g_shutDown = false;
}

// The stream thread's work at each tick of its timer (StreamADPCM)
void Tick()
{
    if (g_shutDown)
    {
        ShutOff();
    }
    CheckStereo();
    for (Stream& s : g_streams)
    {
        if (s.active)
        {
            s.nax = Nax(s.voice);
            s.envx = Envx(s.voice);
        }
    }
    if (g_requestLoad >= 0 && !g_streams[g_requestLoad].active)
    {
        g_requestLoad = -1;
        g_loadFromEE = 0xFFFF;
    }
    if (g_loadFromEE == 0xFFFF || static_cast<u32>(g_requestLoad) != g_loadFromEE)
    {
        // The streams in turn, from either end on alternate ticks: the first with work asks the EE for it
        u32 index = g_direction == 0 ? 0 : g_maxStreams - 1;
        for (u32 n = 0; n < g_maxStreams; n++, index = g_direction == 0 ? index + 1 : index - 1)
        {
            Stream& s = g_streams[index];
            if (g_requestLoad != -1 || !s.active)
            {
                continue;
            }
            if (s.state == Queued)
            {
                if (StartStream(s))
                {
                    g_keyStatus[s.askVoice % Voices] = 2;
                    g_requestLoad = static_cast<s32>(index);
                }
            }
            else if (s.state == Loaded)
            {
                g_requestLoad = static_cast<s32>(index);
            }
            else if (s.state == Playing)
            {
                u32 nax = Nax(s.voice);
                if ((s.half == 0 && nax > s.base + s.halfSize) || (s.half == 1 && nax < s.base + s.halfSize))
                {
                    g_requestLoad = static_cast<s32>(index);
                }
                if (Envx(s.voice) == 0)
                {
                    // Played to its end
                    SetVolume(s.voice, 0, 0);
                    SdSetSwitch(CoreOf(s.voice) | SD_S_KOFF, VoiceBit(s.voice));
                    s.active = false;
                    if (g_trace)
                    {
                        Log("nusound: stream %u (%x) ended", index, s.id);
                    }
                }
            }
        }
        g_loadCounter++;
    }
    else
    {
        u32 index = static_cast<u32>(g_requestLoad);
        g_requestLoad = -1;
        g_loadFromEE = 0xFFFF;
        ServeRequest(index);
    }
    g_direction ^= 1;
}

// TransferResidentSampleBank: the sound effect read from the disc into the sound memory after the last one, as the index'th
// resident one: 0 done, 1 no such sample (the EE's list is over), 2 its file wasn't found
s32 TransferSpotFx(s32 id, u32 index)
{
    int sample = FindSample(id);
    if (sample < 0)
    {
        return 1;
    }
    const FileInfo& file = g_files[sample];
    if (file.size <= VagHeader)
    {
        return 2;
    }
    if (index < MaxSamples)
    {
        g_spotFx[index] = {id, g_spuEnd};
        g_spotFx[index + 1].id = -1;
        // The module loads it through the last stream, whose voice's stream it stops first
        StopStream(g_maxStreams - 1);
        g_streams[g_maxStreams - 1].active = false;
        std::vector<u8> data(file.size - VagHeader);
        ReadBytes(file.lsn, VagHeader, static_cast<u32>(data.size()), data.data());
        SdVoiceTransWrite(g_spuEnd, data.data(), static_cast<u32>(data.size()));
        if (g_trace)
        {
            Log("nusound: sound %x (%.32s) at %x, %zu bytes", id, g_samples[sample].name, g_spuEnd, data.size());
        }
        g_spuEnd += file.size;
    }
    return 0;
}

u32 SpotFxAddress(s32 id)
{
    for (u32 i = 0; i < MaxSamples; i++)
    {
        if (static_cast<s16>(g_spotFx[i].id) == static_cast<s16>(id))
        {
            return g_spotFx[i].address;
        }
    }
    return 0xFFFFFFFF;
}

// InitStreamData: the streams reset, the samples' files found, the timer started
void InitStreams()
{
    g_stereoStarted = false;
    g_stereo1 = g_stereo2 = 0;
    g_stereoPitch = 0;
    for (u32 i = 0; i < MaxStreams; i++)
    {
        Stream& s = g_streams[i];
        s.active = false;
        s.voice = static_cast<u8>(i);
        s.left = s.right = 0;
        s.allocated = false;
        s.half = 0;
        s.state = Idle;
    }
    FindFiles(0);
    SoundSetTimer(TimerSamples, Tick);
}

// AdpcmGetStatus (GetChannelStatus first: a voice plays while its envelope is above 0, unless its stream is being started)
void UpdateStatus()
{
    for (u32 v = 0; v < Voices; v++)
    {
        if (g_keyStatus[v] != 2)
        {
            g_keyStatus[v] = Envx(v) != 0 ? 1 : 0;
        }
    }
    std::memset(g_status, 0, sizeof(g_status));
    u32 active = 0;
    for (u32 i = 0; i < 4; i++)
    {
        const Stream& s = g_streams[i];
        active |= (s.active ? 1u : 0u) << i;
        g_status[StreamVoice + i] = s.voice;
        g_status[StreamId + i] = s.id;
        g_status[StreamNax + i] = static_cast<s32>(s.nax);
        g_status[StreamHalf + i] = s.half;
        g_status[StreamEnvelope + i] = static_cast<s32>(s.envx);
    }
    g_status[StreamActive] = static_cast<s32>(active);
    g_status[SfxTransferEnd] = g_resStatus;
    g_status[StreamBusyStatus] = 0;
    g_status[RequestLoadStatus] = g_requestLoad;
    g_status[LoadCounterStatus] = static_cast<s32>(g_loadCounter);
    g_status[SfxTransferStatus] = 0;
    g_status[ShutoffStatus] = g_shutDown ? 1 : 0;
    g_status[StereoStatus] = g_stereoStarted ? 1 : 0;
    for (u32 word = 0; word < 3; word++)
    {
        u32 bits = 0;
        for (u32 j = 0; j < 16; j++)
        {
            bits = (bits << 2) | g_keyStatus[word * 16 + j];
        }
        g_status[KeyStatusWords + word] = static_cast<s32>(bits);
    }
}

std::string Text(const u16* a, u32 length)
{
    const char* s = reinterpret_cast<const char*>(a);
    return std::string(s, strnlen(s, length * 2));
}

void Command(u32 command, const u16* a, u32 length)
{
    switch (command)
    {
    case 0x00: // play a resident sound effect: sample, voice, volumes, pitch, envelope
        if (g_anyFiles && a[1] < Voices)
        {
            u32 address = SpotFxAddress(a[0]);
            if (address == 0xFFFFFFFF)
            {
                if (Once("nusound sfx " + std::to_string(a[0])))
                {
                    Log("nusound: sound effect %x isn't resident", a[0]);
                }
                break;
            }
            SetVoice(a[1], a[4], a[2], a[3], address, a[5]);
            KeyOn(a[1]);
        }
        break;
    case 0x01: // play a stream: stream, voice (+ bank << 8), volumes, pitch, start, envelope
        if (g_anyFiles)
        {
            AskStream(a[1], a[2], a[3], a[0], a[4], a[5], a[6]);
        }
        break;
    case 0x02: // a voice's volumes
        SetVolume(a[0], a[1], a[2]);
        break;
    case 0x03: // a voice's pitch
        SetPitch(a[0], a[1]);
        break;
    case 0x04: // stop a voice
        if (a[0] < Voices)
        {
            KeyOff(a[0]);
        }
        break;
    case 0x05: // stop a stream (by its voice, or 0x40 + its number)
        if (g_anyFiles)
        {
            StopStream(a[0]);
        }
        break;
    case 0x06: // a resident sound effect into the sound memory: sample, index
        g_resStatus = g_anyFiles ? TransferSpotFx(a[0], a[1]) : 1;
        break;
    case 0x07: // init: the files' path
        g_filePath = Text(a, length);
        g_fileTerm = g_filePath.compare(0, 7, "cdrom0:") == 0 ? ";1" : "";
        SdInit();
        SdSetCoreAttr(SD_C_SPDIF_MODE, 0x800);
        for (u16 core = 0; core < 2; core++)
        {
            SdSetParam(core | SD_P_MVOLL, 0x3FFF);
            SdSetParam(core | SD_P_MVOLR, 0x3FFF);
        }
        g_reverbVoices[0] = g_reverbVoices[1] = 0;
        break;
    case 0x09: // the status, with the stream the EE lets load
        g_loadFromEE = a[0];
        UpdateStatus();
        break;
    case 0x0A: // pause: every voice's pitch 0
        for (u32 v = 0; v < Voices; v++)
        {
            g_pausePitch[v] = SdGetParam(VoiceEntry(v) | SD_VP_PITCH);
            SdSetParam(VoiceEntry(v) | SD_VP_PITCH, 0);
        }
        break;
    case 0x0B: // resume
        for (u32 v = 0; v < Voices; v++)
        {
            SdSetParam(VoiceEntry(v) | SD_VP_PITCH, g_pausePitch[v]);
        }
        break;
    case 0x0C: // init the streams
        InitStreams();
        break;
    case 0x0D: // a stream's buffer: stream, sound memory address, size
    {
        Stream& s = g_streams[a[0] % MaxStreams];
        if (!s.allocated)
        {
            s.allocated = true;
            s.size = (static_cast<u32>(a[3]) << 16) | a[4];
            s.halfSize = s.size / 2;
            s.base = (static_cast<u32>(a[1]) << 16) | a[2];
        }
        break;
    }
    case 0x0E: // where the resident sound effects go
        g_spuEnd = (static_cast<u32>(a[0]) << 16) | a[1];
        break;
    case 0x0F: // a stream's buffer freed
        g_streams[a[0] % MaxStreams].allocated = false;
        break;
    case 0x10: // how many streams
        if (a[0] < 0x30)
        {
            g_maxStreams = a[0];
        }
        break;
    case 0x11: // start a stereo pair together: its voices and pitch
        g_stereo1 = a[0];
        g_stereo2 = a[1];
        g_stereoPitch = a[2];
        g_stereoStarted = true;
        break;
    case 0x12:
        g_stereoStarted = false;
        break;
    case 0x13: // reverb on (libsd's presets)
        if (Once("nusound reverb"))
        {
            Log("nusound: reverb asked for (core %u, mode %u): not done", a[0], a[1]);
        }
        break;
    case 0x14: // reverb off
        SdSetParam((a[0] & 1) | SD_P_EVOLL, 0);
        SdSetParam((a[0] & 1) | SD_P_EVOLR, 0);
        SdSetCoreAttr((a[0] & 1) | SD_C_EFFECT_ENABLE, 0);
        break;
    case 0x15: // reverb's volumes
        SdSetParam((a[0] & 1) | SD_P_EVOLL, a[1]);
        SdSetParam((a[0] & 1) | SD_P_EVOLR, a[2]);
        break;
    case 0x16: // every voice of a core to the reverb, or none (0x17)
    case 0x17:
        g_reverbVoices[a[0] & 1] = command == 0x16 ? 0xFFFFFF : 0;
        SdSetSwitch((a[0] & 1) | SD_S_VMIXEL, g_reverbVoices[a[0] & 1]);
        SdSetSwitch((a[0] & 1) | SD_S_VMIXER, g_reverbVoices[a[0] & 1]);
        break;
    case 0x18: // a voice to the reverb, or not (0x19)
    case 0x19:
        if (a[0] < Voices)
        {
            u32& bits = g_reverbVoices[CoreOf(a[0])];
            bits = command == 0x18 ? bits | VoiceBit(a[0]) : bits & ~VoiceBit(a[0]);
            SdSetSwitch(CoreOf(a[0]) | SD_S_VMIXEL, bits);
            SdSetSwitch(CoreOf(a[0]) | SD_S_VMIXER, bits);
        }
        break;
    case 0x1A: // a sample's SFXINFO
        if (g_sampleCount < MaxSamples)
        {
            std::memcpy(&g_samples[g_sampleCount], a, std::min<size_t>(length * 2, sizeof(SampleInfo)));
            g_sampleCount++;
        }
        break;
    case 0x1B: // the global samples end here: a level's own come after
        g_globalSampleCount = g_sampleCount;
        g_globalSpuEnd = g_spuEnd;
        break;
    case 0x1C: // back to the global samples
        g_sampleCount = g_globalSampleCount;
        for (u32 i = g_globalSampleCount; i < MaxSamples; i++)
        {
            g_samples[i].pitch = 0;
        }
        g_spuEnd = g_globalSpuEnd;
        break;
    case 0x1D: // the level's samples' files found
        FindFiles(g_globalSampleCount);
        break;
    case 0x1E: // the global archive's name
        g_newGlobalDat = Text(a, length);
        break;
    case 0x1F: // the level's archive's name
        g_newLocalDat = Text(a, length);
        break;
    case 0x20: // a stereo pair's pitch
        SetPitch(a[0], a[1]);
        SetPitch(a[0] + 1u, a[1]);
        break;
    case 0x21: // all audio off: done at the stream thread's next tick
        g_shutDown = true;
        break;
    default:
        if (Once("nusound command " + std::to_string(command)))
        {
            Log("nusound: command %x not done", command);
        }
        break;
    }
}

void Serve(RpcCall& c)
{
    auto lock = SoundLock();
    u32 count = c.In16(0);
    u32 at = 2;
    for (u32 n = 0; n < count && at + 4 <= c.inSize; n++)
    {
        u32 command = c.In16(at);
        u32 length = c.In16(at + 2);
        u16 args[512] = {};
        for (u32 k = 0; k < length && k < 512 && at + 4 + k * 2 + 2 <= c.inSize; k++)
        {
            args[k] = c.In16(at + 4 + k * 2);
        }
        if (g_trace && command != 9)
        {
            if (command == 0x1A)
            {
                SampleInfo info = {};
                std::memcpy(&info, args, sizeof(info));
                Log("nusound: sample %.32s: pitch %d, loop %d, id %x", info.name, info.pitch, info.loopInfo, info.id);
            }
            else if (command == 0x07 || command == 0x1E || command == 0x1F)
            {
                Log("nusound: command %x (%u): %s", command, length, Text(args, length).c_str());
            }
            else
            {
                Log("nusound: command %x (%u): %x %x %x %x %x %x %x", command, length, args[0], args[1], args[2], args[3],
                    args[4], args[5], args[6]);
            }
        }
        Command(command, args, length);
        at += 4 + length * 2;
    }
    c.out.assign(c.recvSize, 0);
    std::memcpy(c.out.data(), g_status, std::min<size_t>(c.out.size(), sizeof(g_status)));
}
}

void WocRegisterIopServers() { Ps2::IopRegisterServer(SoundRpcId, "nusound", Serve); }
