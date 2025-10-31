#pragma once
#include <stdint.h>

void init_audio_pipewire(uint8_t* buff, uint8_t version);
void parse_audio_frame_pipewire(uint8_t* buff, opcodeinfo version);