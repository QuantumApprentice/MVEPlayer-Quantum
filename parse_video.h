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
    };
    uint32_t color : 24;    //RGB
};
struct timer_struct {
    uint32_t rate;
    uint8_t subdivision;
};
#pragma pack(pop)

struct video {
    FILE* fileptr = NULL;
    uint8_t* frnt_buffer = NULL;
    uint8_t* back_buffer = NULL;
    uint8_t* pxls = NULL;

    int pitch;
    int buff_size = 0;

    int map_size;
    uint8_t* map_stream = NULL;

    uint8_t block_buffer[64*3];
    int block_pitch = 8*3;
    uint32_t video_texture;

    int file_size;

    palette pal[256];
    timer_struct timer;

    int video_size = 0;
    int render_w;
    int render_h;

    int block_w;
    int block_h;

    int encode_type[0xf];
    int frame_count = 0;
    bool blit_marker[16];
    bool allow_blit[16] = {
        true,       // 0 // blockCopy_0x00
        true,       // 1 // 
        true,       // 2 // cornerCopy_0x02
        true,       // 3 // cornerCopy_0x03
        true,       // 4 // symmetricCopy_0x04
        true,       // 5 // symmetricCopy_0x05
        true,       // 6 // no-op
        true,       // 7 // pattern_0x07
        true,       // 8 // pattern_0x08
        true,       // 9 // pattern_0x09
        true,       // A // pattern_0x0A
        true,       // B // raw_pixels_0x0B
        true,       // C // raw_pixels_0x0C
        true,       // D // raw_pixels_0x0D
        true,       // E // solid_frame_0x0E
        true        // F // dithered_0x0F
    };

    int data_offset_init = 0;
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
