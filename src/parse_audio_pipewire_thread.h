#pragma once
#include <stdint.h>
#include <spa/utils/ringbuffer.h>


void init_audio_pipewire(audio_handle* audio);
void parse_audio_frame_pipewire();
void shutdown_audio_pipewire();
void push_samples(uint8_t* samples, uint32_t n_samples);
void signal_pipewire(uint32_t n_samples);
