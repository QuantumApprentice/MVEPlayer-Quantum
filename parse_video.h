#pragma once
#include <stdio.h>
#include <stdint.h>

struct chunkinfo {
    uint16_t size;
    uint16_t type;
};


#pragma pack(push,1)
union palette {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
    uint32_t color;
};
struct timer_struct {
    uint32_t rate;
    uint8_t subdivision;
};
#pragma pack(pop)

static struct video {
    uint8_t* video_buffer = NULL;
    uint8_t* map_stream = NULL;
    int map_size;
    palette pal[256];
    timer_struct timer;
    int video_size = 0;
    int render_w;
    int render_h;
    int pitch;
    int block_w;
    int block_h;
} video_buffer;

void init_video(FILE* fileptr, chunkinfo info);
void init_video_buffer(uint8_t* buffer, uint8_t version);
void init_video_mode(uint8_t* buffer);
void init_palette(uint8_t* buffer);

void parse_video_chunk(FILE* fileptr, chunkinfo info);
void create_timer(uint8_t* buffer);
void parse_decoding_map(uint8_t* buffer, int size);

void parse_video_data(uint8_t* buffer);
