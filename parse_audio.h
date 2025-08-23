#pragma once
#include <stdio.h>
#include <stdint.h>

void init_audio(uint8_t* chunk);
void parse_audio_frame(uint8_t* buffer);

