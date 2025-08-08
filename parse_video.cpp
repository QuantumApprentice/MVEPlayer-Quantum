#include "parse_video.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

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

void initialize_video_mode(uint8_t* buffer)
{
    videoinit_info initinfo;
    memcpy(&initinfo, buffer, sizeof(initinfo));
    printf("w: %d, h: %d, f: %04x\n", initinfo.w, initinfo.h, initinfo.f);
}

void initialize_video_buffer(uint8_t* buffer, uint8_t version)
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
        printf("vbuff w: %d, vbuff h, %d, vbuff count: %d\n", vbuff2.w, vbuff2.h, vbuff2.count);

        break;}
    default:
        printf("wtf is going on here?\n");
        break;
    }

    printf("video buffer size: %d\n", buff_size);

}


void init_video(FILE* fileptr, chunkinfo* info)
{
    uint8_t* wut = (uint8_t*)calloc(1, info->size);
    fread(wut, info->size, 1, fileptr);
    printf("chunk -- size: %d type: %d\n", info->size, info->type);

    int offset = 0;
    while (offset < info->size)
    {
        opcodeinfo op;
        memcpy(&op, &wut[offset], sizeof(op));
        printf("op -- len: %d, type: %04x, ver: %d\n", op.size, op.type, op.version);

        offset += 4;

        switch (op.type)
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
            initialize_video_buffer(&wut[offset], op.version);
            break;
        case 0x06:
            break;
        case 0x07:
            break;
        case 0x08:
            break;
        case 0x09:
            break;
        case 0x0A:  //Initialize Video Mode
            initialize_video_mode(&wut[offset]);
            break;
        case 0x0B:
            break;
        case 0x0C:
            break;
        case 0x0D:
            break;
        case 0x0E:
            break;
        case 0x0F:
            break;
        case 0x10:
            break;
        case 0x11:
            break;
        case 0x12:
            break;
        case 0x13:
            break;
        case 0x14:
            break;
        case 0x15:
            break;

        default:
            break;
        }

        offset += op.size;

    }



    // memcpy(&op, wut+5, sizeof(op));
    // printf("op -- len: %d, type: %04x, ver: %d\n", op.size, op.type, op.version);




    free(wut);
}