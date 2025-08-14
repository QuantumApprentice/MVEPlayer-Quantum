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
        // uint8_t a;
    };
    uint32_t color : 24;    //RGB
};
struct timer_struct {
    uint32_t rate;
    uint8_t subdivision;
};
#pragma pack(pop)

struct video {
    uint8_t* video_buffer = NULL;
    uint8_t* back_buffer = NULL;
    uint32_t video_texture;
    int file_size;
    uint8_t* map_stream = NULL;
    int map_size;
    palette pal[256];
    timer_struct timer;
    int video_size = 0;
    int buff_size = 0;
    int render_w;
    int render_h;
    int pitch;
    int block_w;
    int block_h;
    int encode_type[0xf+1];
};
extern video video_buffer;

void init_video(FILE* fileptr, chunkinfo info);
void init_video_buffer(uint8_t* buffer, uint8_t version);
void init_video_mode(uint8_t* buffer);
void init_palette(uint8_t* buffer);

void parse_video_chunk(FILE* fileptr, chunkinfo info);
void create_timer(uint8_t* buffer);
void parse_decoding_map(uint8_t* buffer, int size);

void parse_video_data(uint8_t* buffer);
void send_buffer_to_display(uint8_t* buffer);
