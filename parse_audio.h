#pragma once
#include <stdio.h>
#include <stdint.h>

void init_audio(uint8_t* chunk);
void fill_audio(int frequency);
void parse_audio_frame(uint8_t* buffer);
void shutdown_audio();
