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

struct palette {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

static struct video_buff {
    uint8_t* video_buffer = NULL;
    struct palette pal[256];
    int size = 0;
    int render_w;
    int render_h;
    int block_w;
    int block_h;
} video_buffer;

void init_video(FILE* fileptr, chunkinfo* info);
