#pragma once
#include <stdio.h>
#include <stdint.h>

#include "parse_video.h"

// void parse_op(uint8_t* chunk, chunkinfo info);
void init_audio(uint8_t* buff, uint8_t version);
void fill_audio(int frequency);
void parse_audio_frame(uint8_t* buffer, opcodeinfo op);
void shutdown_audio();