#include "parse_video.h"
#include "parse_opcodes.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

bool debug = false;

#pragma pack(push, 1)
struct videoinit_info
{
    uint16_t w;
    uint16_t h;
    uint16_t f;
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

        video_buffer.block_w = vbuff2.w;
        video_buffer.block_h = vbuff2.h;
        video_buffer.size    = buff_size;

        video_buffer.video_buffer = (uint8_t*)malloc(buff_size);

        break;}
    default:
        printf("wtf is going on here?\n");
        break;
    }
    if (debug) {
        printf("video buffer size: %d\n", buff_size);
    }

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
            i++;
        }
    }
    memcpy(&video_buffer.pal[i], &buffer[4], pal_info.count*3);
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