#pragma once
#include <stdio.h>
#include <stdint.h>

struct chunkinfo {
    uint16_t size;
    uint16_t type;
};

struct opcodeinfo {
    uint16_t size;
    uint8_t  type;
    uint8_t version;
};

void init_video(FILE* fileptr, chunkinfo* info);
