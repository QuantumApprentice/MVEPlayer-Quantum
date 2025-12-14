#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

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
bool init_sdl()
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
        .freq     = 22050,
        .format   = AUDIO_S16LSB,
        .channels = 2,
        .samples  = 2048,
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

void play_sdl()
{
    SDL_PauseAudioDevice(playbackID, false);
}

void close_sdl()
{
    SDL_Quit();
}
