#pragma once
#include <stdint.h>

#pragma pack(push,1)
struct audio_frame {
    uint16_t index;
    uint16_t mask;
    uint16_t length;
    uint8_t data[];
};
#pragma pack(pop)

void init_audio(uint8_t* buff, uint8_t version);
void decompress_8(uint8_t* buff, int len);
void decompress_16(uint8_t* buff, int len);
void parse_audio_frame(uint8_t* buff, opcodeinfo op);
void shutdown_audio();