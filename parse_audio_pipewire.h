#pragma once
#include <stdint.h>
#include <spa/utils/ringbuffer.h>

#define DEFAULT_CHANNELS    (2)
#define BUFFER_SIZE         (16*1024)

struct pw_data {
    struct pw_main_loop* main_loop;
    struct pw_loop*      loop;
    struct pw_stream*    stream;

    float accumulator;

    struct spa_source* refill_event;

    struct spa_ringbuffer ring;
    float buff[BUFFER_SIZE * DEFAULT_CHANNELS];
};

void init_audio_pipewire(uint8_t* buff, uint8_t version);
void parse_audio_frame_pipewire(uint8_t* buff, opcodeinfo version);
void shutdown_audio_pipewire();