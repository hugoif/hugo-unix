#include <SDL.h>
#include <SDL_audio.h>
#include <algorithm>
#include <memory>
#include <unistd.h>

#include "Aulib/DecoderFluidsynth.h"
#include "Aulib/DecoderModplug.h"
#include "Aulib/DecoderMpg123.h"
#include "Aulib/DecoderOpenmpt.h"
#include "Aulib/DecoderSndfile.h"
#include "Aulib/DecoderXmp.h"
#include "Aulib/ResamplerSpeex.h"
#include "Aulib/Stream.h"
#include "aulib.h"

extern "C" {
#include "heheader.h"
}
#include "rwopsbundle.h"
#include "soundfont_data.h"

static std::unique_ptr<Aulib::Stream>& musicStream()
{
    static auto p = std::unique_ptr<Aulib::Stream>();
    return p;
}

static std::unique_ptr<Aulib::Stream>& sampleStream()
{
    static auto p = std::unique_ptr<Aulib::Stream>();
    return p;
}

static Aulib::DecoderFluidsynth*& fsynthDec()
{
    static Aulib::DecoderFluidsynth* p = nullptr;
    return p;
}

static float convertHugoVolume(int hugoVol)
{
    return std::min(std::max(0, hugoVol), 100) / 100.f;
}

extern "C" void initSoundEngine()
{
    static bool is_initialized = false;

    if (is_initialized) {
        return;
    }

    // Initialize only the audio part of SDL.
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "Unable to initialize sound system: %s", SDL_GetError());
        exit(1);
    }

    // Initialize audio with 44.1kHz, 16 bit, 2 channels (stereo) and a 4k chunk size.
    // TODO: Make this configurable?
    if (not Aulib::init(44100, AUDIO_S16SYS, 2, 4096)) {
        fprintf(stderr, "Unable to initialize audio: %s", SDL_GetError());
        exit(1);
    }

    is_initialized = true;
}

extern "C" void closeSoundEngine()
{
    if (musicStream()) {
        musicStream()->stop();
        musicStream().reset();
    }
    if (sampleStream()) {
        sampleStream()->stop();
        sampleStream().reset();
    }
    Aulib::quit();
    SDL_Quit();
}

static bool playStream(HUGO_FILE infile, long reslength, char loop_flag, bool isMusic)
{
    initSoundEngine();

    auto& stream = isMusic ? musicStream() : sampleStream();
    std::unique_ptr<Aulib::Decoder> decoder;

    if (stream) {
        stream->stop();
        stream.reset();
    }

    // Create an RWops for the embedded media resource.
    SDL_RWops* rwops = RWFromMediaBundle(infile, reslength);
    if (rwops == nullptr) {
        fclose(infile);
        return false;
    }

    if (not isMusic) {
        decoder = std::make_unique<Aulib::DecoderSndfile>();
    } else {
        switch (resource_type) {
        case MIDI_R: {
            auto fsdec = std::make_unique<Aulib::DecoderFluidsynth>();
            auto* sf2_rwops = SDL_RWFromConstMem(soundfont_data, sizeof(soundfont_data));
            fsdec->loadSoundfont(sf2_rwops);
            fsdec->setGain(0.6f);
            fsynthDec() = fsdec.get();
            decoder = std::move(fsdec);
            break;
        }
        case XM_R:
        case S3M_R:
        case MOD_R: {
            using ModDecoder_type =
#if USE_DEC_OPENMPT
                Aulib::DecoderOpenmpt;
#elif USE_DEC_XMP
                Aulib::DecoderXmp;
#elif USE_DEC_MODPLUG
                Aulib::DecoderModPlug;
#endif
            decoder = std::make_unique<ModDecoder_type>();
            fsynthDec() = nullptr;
            break;
        }
        case MP3_R:
            decoder = std::make_unique<Aulib::DecoderMpg123>();
            fsynthDec() = nullptr;
            break;
        default:
            SDL_RWclose(rwops);
            return false;
        }
    }

    stream = std::make_unique<Aulib::Stream>(rwops, std::move(decoder),
                                             std::make_unique<Aulib::ResamplerSpeex>(), true);
    if (stream->open()) {
        // Start playing the stream. Loop forever if 'loop_flag' is true. Otherwise, just play it
        // once.
        if (stream->play(loop_flag ? 0 : 1)) {
            return true;
        }
    }

    stream.reset();
    fsynthDec() = nullptr;
    return false;
}

extern "C" int hugo_playmusic(HUGO_FILE infile, long reslength, char loop_flag)
{
    return playStream(infile, reslength, loop_flag, true);
}

extern "C" void hugo_musicvolume(int vol)
{
    if (musicStream()) {
        musicStream()->setVolume(convertHugoVolume(vol));
    }
}

extern "C" void hugo_stopmusic(void)
{
    if (musicStream()) {
        musicStream()->stop();
    }
}

extern "C" int hugo_playsample(HUGO_FILE infile, long reslength, char loop_flag)
{
    return playStream(infile, reslength, loop_flag, false);
}

extern "C" void hugo_samplevolume(int vol)
{
    if (musicStream()) {
        musicStream()->setVolume(convertHugoVolume(vol));
    }
}

extern "C" void hugo_stopsample(void)
{
    if (sampleStream()) {
        sampleStream()->stop();
    }
}
