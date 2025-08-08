#pragma once
#include <stdio.h>
#include <stdint.h>

struct chunkinfo {
    uint16_t size;
    uint16_t type;
};


#pragma pack(push,1)
struct palette {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};
struct timer_struct {
    uint32_t rate;
    uint8_t subdivision;
};
#pragma pack(pop)

static struct video_buff {
    uint8_t* video_buffer = NULL;
    struct palette pal[256];
    timer_struct timer;
    int size = 0;
    int render_w;
    int render_h;
    int block_w;
    int block_h;
} video_buffer;

void init_video(FILE* fileptr, chunkinfo info);
void init_video_buffer(uint8_t* buffer, uint8_t version);
void init_video_mode(uint8_t* buffer);
void init_palette(uint8_t* buffer);

void parse_video_chunk(FILE* fileptr, chunkinfo info);
void create_timer(uint8_t* buffer);
