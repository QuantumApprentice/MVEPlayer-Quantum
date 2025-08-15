#include "parse_video.h"
#include "parse_opcodes.h"
#include "MiniSDL.h"
#include "io_OpenGL.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

bool debug = false;
// extern video video_buffer;

#pragma pack(push, 1)
struct videoinit_info
{
    uint16_t w;
    uint16_t h;
    uint16_t f; //flags
};
struct videoinit_buffer_v0
{
    uint16_t w;
    uint16_t h;
};
struct videoinit_buffer_v1
{
    uint16_t w;
    uint16_t h;
    uint16_t count;
};
struct videoinit_buffer_v2
{
    uint16_t w;
    uint16_t h;
    uint16_t count;
    uint16_t color;
};
#pragma pack(pop)

void init_video_mode(uint8_t* buffer)
{
    videoinit_info initinfo;
    memcpy(&initinfo, buffer, sizeof(initinfo));
    if (debug) {
        printf("w: %d, h: %d, f: %04x\n", initinfo.w, initinfo.h, initinfo.f);
    }
    video_buffer.render_w = initinfo.w;
    video_buffer.render_h = initinfo.h;
    video_buffer.pitch    = initinfo.w * 3;
}

void init_video_buffer(uint8_t* buffer, uint8_t version)
{
    int buff_size = 0;
    switch (version)
    {
    case 0:{
        videoinit_buffer_v0 vbuff0;
        memcpy(&vbuff0, buffer, sizeof(vbuff0));
        buff_size = 2* vbuff0.w * vbuff0.h;
        break;}
    case 1:{
        videoinit_buffer_v1 vbuff1;
        memcpy(&vbuff1, buffer, sizeof(vbuff1));
        buff_size = 2* vbuff1.w * vbuff1.h * vbuff1.count;
        break;}
    case 2:{
        videoinit_buffer_v2 vbuff2;
        memcpy(&vbuff2, buffer, sizeof(vbuff2));
        buff_size = 2* vbuff2.w * vbuff2.h * vbuff2.count;

        if (debug) {
            printf("vbuff w: %d, vbuff h, %d, vbuff count: %d\n", vbuff2.w, vbuff2.h, vbuff2.count);
        }

        video_buffer.block_w   = vbuff2.w;
        video_buffer.block_h   = vbuff2.h;
        video_buffer.buff_size = buff_size; //what is this buff_size actually for?

        break;}
    default:
        printf("wtf is going on here?\n");
        break;
    }
    if (debug) {
        printf("video buffer size: %d\n", buff_size);
    }

    video_buffer.video_buffer = (uint8_t*)malloc(video_buffer.render_w*video_buffer.render_h*3);
    video_buffer.video_texture = gen_texture(video_buffer.render_w, video_buffer.render_h, video_buffer.video_buffer);
}

void init_palette(uint8_t* buffer)
{
    struct pal_info {
        uint16_t start;
        uint16_t count;
        uint8_t data[];
    } pal_info;
    memcpy(&pal_info, buffer, sizeof(pal_info));

    if (debug) {
        printf("pal start: %d, pal count: %d\n", pal_info.start, pal_info.count);
    }
    int i = 0;
    if (pal_info.start != 0) {
        while (i < pal_info.start)
        {
            video_buffer.pal[i].r = 0;
            video_buffer.pal[i].g = 0;
            video_buffer.pal[i].b = 0;
            // video_buffer.pal[i].a = 255;
            i++;
        }
    }
    memcpy(&video_buffer.pal[i], &buffer[4], pal_info.count*3);
    // video_buffer.pal[i].a = 255;


    if (debug) {
        printf("palette colors:\n");
        for (int i = 0; i < pal_info.count + pal_info.start; i++)
        {
            printf("i:%d r:%d g:%d b:%d\n", i, video_buffer.pal[i].r, video_buffer.pal[i].g, video_buffer.pal[i].b);
        }
    }
}

void init_video(FILE* fileptr, chunkinfo info)
{
    uint8_t* wut = (uint8_t*)calloc(1, info.size);
    fread(wut, info.size, 1, fileptr);
    printf("chunk -- size: %d type: %d\n", info.size, info.type);

    int offset = 0;
    while (offset < info.size)
    {
        opcodeinfo op;
        memcpy(&op, &wut[offset], sizeof(op));
        printf("op -- len: %d, type: %04x, ver: %d\n", op.size, op.type, op.version);

        offset += 4;
        parse_opcode(op, &wut[offset]);
        offset += op.size;
    }
    free(wut);
}

void create_timer(uint8_t* buffer)
{
    memcpy(&video_buffer.timer, buffer, sizeof(video_buffer.timer));

    if (debug) {
        printf("rate: %d, subdivision: %d\n", video_buffer.timer.rate, video_buffer.timer.subdivision);
    }
}

void send_buffer_to_display(uint8_t* buffer)
{
    struct pal_info {
        uint16_t start;
        uint16_t count;
        uint16_t unknown;
    } info;

    memcpy(&info, buffer, sizeof(info));

    printf("pal start: %d, pal count: %d, unkown: %d\n", info.start, info.count, info.unknown);


    //TODO: should I use palette to convert pixels here?
    //      instead of during the paint process?

    GLuint tex = video_buffer.video_texture;
    int w      = video_buffer.render_w;
    int h      = video_buffer.render_h;
    //TODO: swap buffers
    uint8_t* pxls = video_buffer.video_buffer;
    blit_to_texture(tex, pxls, w, h);
}

void parse_video_chunk(FILE* fileptr, chunkinfo info)
{
    uint8_t* wut = (uint8_t*)calloc(1, info.size);
    fread(wut, info.size, 1, fileptr);
    printf("chunk -- size: %d type: %d\n", info.size, info.type);

    int offset = 0;
    while (offset < info.size)
    {
        opcodeinfo op;
        memcpy(&op, &wut[offset], sizeof(op));
        printf("op -- len: %d, type: %04x, ver: %d\n", op.size, op.type, op.version);

        offset += 4;
        parse_opcode(op, &wut[offset]);
        offset += op.size;
    }
    printf("done processing video chunk\n");
    free(wut);
}

void parse_decoding_map(uint8_t* buffer, int size)
{
    video_buffer.map_size   = size;
    video_buffer.map_stream = (uint8_t*)malloc(size);
    memcpy(video_buffer.map_stream, buffer, size);
}

//set the whole frame a solid color?
void solid_frame(uint8_t pal_index, video* video_buffer, uint8_t* dst_buff)
{
    Rect dst_rect;
    dst_rect.w = 8;
    dst_rect.h = 8;
    dst_rect.x = 0;
    dst_rect.y = 0;

    palette color = video_buffer->pal[pal_index];
    int pitch = video_buffer->pitch;

    PaintSurface(dst_buff, pitch, dst_rect, color);
}

void pattern_0x07(uint8_t* data_stream, video* video, uint8_t* dst_buff)
{
    uint8_t P0 = data_stream[0];
    uint8_t P1 = data_stream[1];

    uint8_t P[2];
    P[0] = data_stream[0];
    P[1] = data_stream[1];

    int pitch  = video->pitch;
    palette* pal = video->pal;

    uint8_t byte[8];
    if (P0 <= P1) {
        for (int i = 0; i < 8; i++)
        {
            byte[i] = data_stream[i+2];
        }

        uint8_t mask = 128;
        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 8; x++)
            {
                bool indx = byte[y] & (mask >> x);

                int pos = y*pitch + x;
                palette color = pal[P[indx]];

                dst_buff[pos + 0] = color.r;
                dst_buff[pos + 1] = color.g;
                dst_buff[pos + 2] = color.b;

                dst_buff[pos + 3] = 255;
            }
        }

    } else {
        byte[0] = data_stream[2];
        byte[1] = data_stream[3];
        // 22 22 22 22 22 22 22 22 ; f == 1 0 1 1
        // 22 22 22 22 22 22 22 22 ; 
        // 22 22 22 22 22 22 22 22 ; f == 1 1 1 1
        // 22 22 22 22 22 22 22 22 ; 
        // 22 22 11 11 11 11 11 11 ; 8 == 1 0 0 0
        // 22 22 11 11 11 11 11 11 ; 
        // 11 11 11 11 11 11 22 22 ; 1 == 0 0 0 1
        // 11 11 11 11 11 11 22 22 ;

        int byte_index = 0;
        uint8_t mask = 128;
        uint8_t mask_offset = 0;
        for (int y = 0; y < 8; y+=2)
        {
            for (int x = 0; x < 8; x+=2)
            {
                bool pal_index = byte[byte_index] & (mask >> mask_offset);
                mask_offset++;
                if (mask_offset >= 8) {
                    mask_offset = 0;
                }
                palette color = pal[P[pal_index]];

                int pos = y*pitch + x;
                Rect out = {
                    out.w = 2,
                    out.h = 2,
                    out.x = x,
                    out.y = y
                };

                PaintSurface(dst_buff, pitch, out, color);
            }
        }
    }
}

void pattern_0x08(uint8_t* data_stream, video* video, uint8_t* dst_buff)
{
#pragma pack(push,1)
    struct quadrant {
        uint8_t P0; uint8_t P1; uint8_t B0; uint8_t B1;
    };
#pragma pack(pop)
    quadrant quad[4];

    memcpy(&quad, data_stream, sizeof(quad));

    uint8_t buffer[16*3] = {
        255,255,255,  255,255,255,  255,255,255,  255,255,255,
        255,255,255,  255,255,255,  255,255,255,  255,255,255,
        255,255,255,  255,255,255,  255,255,255,  255,255,255,
        255,255,255,  255,255,255,  255,255,255,  255,255,255,
    };
    int buff_pitch = 4*3;

    palette* pal = video->pal;

    //if P0 <= P1 we get all 4 corners
    uint8_t mask_offset = 0;
    if (quad[0].P0 <= quad[0].P1) {
        int q = 0;
        for (int y = 0; y < 4; y++)
        {
            for (int x = 0; x < 4; x++)
            {
                uint8_t mask = 128 >> mask_offset++;
                if (mask_offset >= 8) {
                    mask_offset = 0;
                }
                bool indx;
                if (y < 2) {
                    indx = quad[q].B0 & mask;
                } else {
                    indx = quad[q].B1 & mask;
                }
                palette color = indx ? pal[quad[q].P0] : pal[quad[q].P1];

                buffer[y*buff_pitch + x*3 +0] = color.r;
                buffer[y*buff_pitch + x*3 +1] = color.g;
                buffer[y*buff_pitch + x*3 +2] = color.b;

                Rect src_rect = {
                    .x = 0,
                    .y = 0,
                    .w = 8,
                    .h = 8,
                };
                Rect dst_rect = {
                    .x = x,
                    .y = y,
                    .w = 8,
                    .h = 8,
                };
                if (mask == 0xF0) {
                    dst_rect.x = x+4;
                }

                BlitSurface(&buffer[0], src_rect, dst_buff, dst_rect, 4*3, video->pitch);
            }
        
        }
        
    }
    


}


int parse_video_encode(uint8_t* data_stream, uint8_t* video, uint8_t op)
{
    int offset = 0;
    video_buffer.encode_type[op]++;
    switch (op)
    {
    case 0x00:
        break;
    case 0x01:
        break;
    case 0x02:
        break;
    case 0x03:
        break;
    case 0x04:
        break;
    case 0x05:
        break;
    case 0x06:
        break;
    case 0x07:
        pattern_0x07(data_stream, &video_buffer, video);
        break;
    case 0x08:
        pattern_0x08(data_stream, &video_buffer, video);
        break;
    case 0x09:
        break;
    case 0x0A:
        break;
    case 0x0B:
        break;
    case 0x0C:
        break;
    case 0x0D:
        break;
    case 0x0E:
        solid_frame(data_stream[0], &video_buffer, video);
        break;
    case 0x0F:
        break;

    default:
        break;
    }

        offset = 8*3;
    return offset;
}

void parse_video_data(uint8_t* buffer)
{
    uint8_t* map_stream = video_buffer.map_stream;
    uint8_t* data_stream = buffer;

    uint8_t* video = video_buffer.video_buffer;

    uint8_t mask = 0x0F;
    int offset   = 0;
    int pitch    = video_buffer.pitch;
    int y        = 0;

    for (int i = 0; i < video_buffer.map_size*2; i++)
    {
        mask = ~mask;
        uint8_t enc = map_stream[i/2] & mask;
        if (mask == 0xF0) {
            enc >>= 4;
        }

        offset += parse_video_encode(&data_stream[i], &video[y*pitch + offset], enc);
        if (offset >= video_buffer.pitch) {
            offset = 0;
            y+=8;
        }
    }
}