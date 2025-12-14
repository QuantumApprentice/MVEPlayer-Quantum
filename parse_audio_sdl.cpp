#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "parse_audio_sdl.h"


    Mix_Music* gMusic   = NULL;
    Mix_Chunk* gScratch = NULL;
    Mix_Chunk* gHigh    = NULL;
    Mix_Chunk* gMedium  = NULL;
    Mix_Chunk* gLow     = NULL;

uint8_t* buff_ptr;
uint32_t buffByteSize;
SDL_AudioDeviceID playbackID;
// int buff_pos;

void playbackCB(void* userdata, uint8_t* stream, int len)
{
    memcpy(stream, buff_ptr, len);
    // memcpy(stream, &buff_ptr[buff_pos], len);

    // buff_pos += len;
}

//Initialize SDL_mixer
bool init_sdl(uint8_t* buff, int size)
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
    } else {
        // int bytesPerSample = playbackSpec_rec.channels * (SDL_AUDIO_BITSIZE(playbackSpec_rec.format)/8);
        // int bytesPerSecond = playbackSpec_rec.freq * bytesPerSample;
        buff_ptr     = buff;
        buffByteSize = size;
    }

    // if (Mix_OpenAudio(22050, AUDIO_S16LSB, 2, 2048) < 0) {
    //     printf("Error initializing SDL! SDL_mixer Error: %s\n", Mix_GetError());
    //     return false;
    // }

    // gMusic = Mix_LoadMUS("dependencies/Pop3.wav");
    // if (gMusic == NULL) {
    //     printf("fuuuuuuck - Mix_LoadMUS didn't load the wav file\n");
    //     return false;
    // }
    // gScratch = Mix_LoadWAV("dependencies/Pop3.wav");
    // if (gMusic == NULL) {
    //     printf("fuuuuuuck - Mix_LoadWAV didn't load the wav file\n");
    //     return false;
    // }

    return true;
}

void play_sdl()
{

    SDL_PauseAudioDevice(playbackID, false);

    // printf("attempting to play SDL audio\n");
    // Mix_PlayMusic(gMusic, -1);
}

void close_sdl()
{
    Mix_FreeMusic(gMusic   );

    Mix_FreeChunk(gScratch );
    Mix_FreeChunk(gHigh    );
    Mix_FreeChunk(gMedium  );
    Mix_FreeChunk(gLow     );
    gMusic   = NULL;
    gScratch = NULL;
    gHigh    = NULL;
    gMedium  = NULL;
    gLow     = NULL;

    Mix_Quit();
    SDL_Quit();
}
