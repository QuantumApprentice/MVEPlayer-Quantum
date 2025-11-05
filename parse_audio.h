#pragma once
#include <stdint.h>
#include "parse_video.h"

struct audio_handle {
    int16_t* audio_buff    = NULL;
    int audio_buff_size    = 0;
    int audio_min_buff_len = 0;
    float audio_volume     = 0.5f;
    int audio_freq         = 210;
    uint32_t audio_rate    = 48000;
    uint32_t audio_channels = 2;
    int audio_samples_per_frame;
    int audio_bits         = 0;
    int audio_compress     = 0;
    // int audio_calc_rate = 0;
};

void init_audio(uint8_t* buff, uint8_t version);
void decompress_8(uint8_t* buff, int len);
void decompress_16(uint8_t* buff, int len);
void parse_audio_frame(uint8_t* buff, opcodeinfo op);
void shutdown_audio();