#pragma once
#include <stdio.h>
#include <stdint.h>
#include "parse_opcodes.h"
#include "parse_audio_pipewire.h"

#include <alsa/asoundlib.h>
#include <pipewire/pipewire.h>

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

#define ALSA        (0)
#define PIPEWIRE    (1)
struct video {
    //files
    bool file_drop_frame = false;
    char* filename = NULL;
    FILE* fileptr = NULL;
    //video
    uint8_t* frnt_buffer = NULL;
    uint8_t* back_buffer = NULL;
    uint8_t* pxls = NULL;
    int pitch;

    //audio
    int16_t* audio_buff    = NULL;
    int audio_buff_size    = 0;
    int audio_min_buff_len = 0;
    float audio_volume     = 0.5f;
    int audio_freq         = 210;
    uint32_t audio_rate    = 48000;
    uint32_t audio_channels = 2;
    int audio_samples_per_frame;
    int audio_bits         = 0;
    int audio_compress     = 0;
    // int audio_calc_rate = 0;

    uint32_t seconds    = 2;


    int audio_pipe     = PIPEWIRE;
    //ALSA
    snd_pcm_t* pcm_handle;
    snd_pcm_uframes_t frames;
    snd_pcm_hw_params_t* audio_params;
    //Pipewire

    int map_size;
    uint8_t* map_stream = NULL;

    uint8_t block_buffer[64*3];
    int block_pitch = 8*3;
    uint32_t video_texture;

    int file_size;

    palette pal[256];
    timer_struct timer;

    int video_size = 0;
    int window_w;
    int window_h;

    int render_w;
    int render_h;

    int block_w;
    int block_h;

    int v_chunk_cnt_total = 0;
    int v_chunk_cnt_last  = 0;
    int v_chunk_per_frame = 0;
    int encode_type[0x0f+1];
    int opcode_type[0x15+1];
    int frame_count = 0;
    bool blit_marker[16];
    bool allow_blit[16] = {
        true,       // 0 // blockCopy_0x00
        true,       // 1 // no-op
        true,       // 2 // cornerCopy_0x02
        true,       // 3 // cornerCopy_0x03
        true,       // 4 // symmetricCopy_0x04
        true,       // 5 // symmetricCopy_0x05
        true,       // 6 // no-op?
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

    uint8_t encode_bits = 0x0F;
    int data_offset_init = 14;
    int data_offset[0xF+1];
};
extern video video_buffer;

void init_video(uint8_t* chunk, chunkinfo info);
void init_video_buffer(uint8_t* buffer, uint8_t version);
void init_video_mode(uint8_t* buffer);
void init_palette(uint8_t* buffer);

bool parse_video_chunk(uint8_t* chunk, chunkinfo info);
void create_timer(uint8_t* buffer);
void parse_decoding_map(uint8_t* buffer, int size);

void parse_video_data(uint8_t* buffer);
void send_buffer_to_display(uint8_t* buffer);


int blockCopy_0x00(uint8_t* data_stream, uint8_t* dst_buff, int offset_x, int offset_y, bool blit, bool mark);
int cornerCopy_0x02(uint8_t* data_stream, int x_offset, int y_offset, uint8_t* dst_buff, bool blit, bool mark);
int cornerCopy_0x03(uint8_t* data_stream, int x_offset, int y_offset, uint8_t* dst_buff, bool blit, bool mark);
int symmetricCopy_0x04(uint8_t* data_stream, int x_offset, int y_offset, uint8_t* dst_buff, bool blit, bool mark);
int symmetricCopy_0x05(uint8_t* data_stream, int x_offset, int y_offset, uint8_t* dst_buff, bool blit, bool mark);
int pattern_0x07(uint8_t* data_stream, video* video, uint8_t* dst_buff, bool blit, bool mark);
int pattern_0x08(uint8_t* data_stream, uint8_t* dst_buff, bool blit, bool mark);
int pattern_0x09(uint8_t* data_stream, video* video, uint8_t* dst_buff, bool blit, bool mark);
int pattern_0x0A(uint8_t* data_stream, video* video, uint8_t* dst_buff, bool blit, bool mark);
int raw_pixels_0x0B(uint8_t* data_stream, video* video_buffer, uint8_t* dst_buff, bool blit, bool mark);
int raw_pixels_0x0C(uint8_t* data_stream, video* video_buffer, uint8_t* dst_buff, bool blit, bool mark);
int raw_pixels_0x0D(uint8_t* data_stream, video*video_buffer, uint8_t* dst_buff, bool blit, bool mark);
int solid_frame_0x0E(uint8_t* data_stream, video* video_buffer, uint8_t* dst_buff, bool blit, bool mark);
int dithered_0x0F(uint8_t* data_stream, video*video_buffer, uint8_t* dst_buff, bool blit, bool mark);