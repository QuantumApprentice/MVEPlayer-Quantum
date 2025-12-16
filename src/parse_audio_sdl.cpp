#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "parse_audio.h"
#include "parse_audio_sdl.h"
#include "ring_buffer.h"

SDL_AudioDeviceID playbackID;

void playbackCB(void* userdata, uint8_t* stream, int len)
{
    int copied = copy_from_ring(stream, len);
    if (copied != len) {
        printf("Error: Read mismatch. Requested: %d Read: %d\n", len, copied);
    }
}

//Initialize SDL_mixer
bool init_sdl(audio_handle* audio)
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("Error initializing SDL! SDL Error: %s\n", SDL_GetError());
        return false;
    }


    int audioDeviceCount = SDL_GetNumAudioDevices(false);
    if (audioDeviceCount < 1) {
        printf( "Unable to enumerate audio devices! SDL Error: %s\n", SDL_GetError() );
        return false;
    }
    for (int i = 0; i < audioDeviceCount; i++)
    {
        printf("SDL Audio Device Name: %d : %s\n", i, SDL_GetAudioDeviceName(i,false));
    }
    SDL_AudioSpec playbackSpec_want;
    SDL_AudioSpec playbackSpec_rec;
    SDL_zero(playbackSpec_want);
    playbackSpec_want = {
        .freq     = (int)audio->audio_rate,
        .format   = (uint16_t)(audio->audio_bits == 16 ? AUDIO_S16LSB : AUDIO_S8),
        .channels = (uint8_t)audio->audio_channels,
        .samples  = (uint16_t)audio->audio_samples_per_frame,
        .callback = playbackCB
    };

    playbackID = SDL_OpenAudioDevice(
        NULL,
        false,
        &playbackSpec_want,
        &playbackSpec_rec,
        SDL_AUDIO_ALLOW_FORMAT_CHANGE
    );

    if (playbackID == 0) {
        printf("Failed to open SDL audio playback device. SDL Error: %s", SDL_GetError());
        return false;
    }

    return true;
}

void play_sdl(bool pause)
{
    SDL_PauseAudioDevice(playbackID, pause);
}

void close_sdl()
{
    SDL_Quit();
}
