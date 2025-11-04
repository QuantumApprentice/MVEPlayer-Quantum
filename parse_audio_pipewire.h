#pragma once
#include <stdint.h>
#include <spa/utils/ringbuffer.h>


void init_audio_pipewire(uint8_t* buff, uint8_t version);
void parse_audio_frame_pipewire();
void shutdown_audio_pipewire();