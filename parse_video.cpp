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

    video_buffer.frnt_buffer = (uint8_t*)calloc(1,video_buffer.render_w*video_buffer.render_h*3);
    video_buffer.back_buffer = (uint8_t*)calloc(1,video_buffer.render_w*video_buffer.render_h*3);
    video_buffer.pxls = video_buffer.frnt_buffer;
    video_buffer.video_texture = gen_texture(video_buffer.render_w, video_buffer.render_h, video_buffer.pxls);
}

void init_palette(uint8_t* buffer)
{
    struct PAL_Info {
        uint16_t start;
        uint16_t count;
    };

    PAL_Info pal_info;
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
    // memcpy(&video_buffer.pal[i], &buffer[4], pal_info.count*3);
    for (int i = 0; i < pal_info.count; i++)
    {   //shift to get the palette color correct
        video_buffer.pal[pal_info.start + i].r = buffer[i*3 + 4 + 0] << 2;
        video_buffer.pal[pal_info.start + i].g = buffer[i*3 + 4 + 1] << 2;
        video_buffer.pal[pal_info.start + i].b = buffer[i*3 + 4 + 2] << 2;
    }


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
    uint8_t* chunk = (uint8_t*)calloc(1, info.size);
    fread(chunk, info.size, 1, fileptr);
    printf("chunk -- size: %d type: %d\n", info.size, info.type);

    int offset = 0;
    while (offset < info.size)
    {
        opcodeinfo op;
        memcpy(&op, &chunk[offset], sizeof(op));
        printf("op -- len: %d, type: 0x%02X, ver: %d\n", op.size, op.type, op.version);

        offset += 4;
        parse_opcode(op, &chunk[offset]);
        offset += op.size;
    }
    free(chunk);
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

    //TODO: should I use palette to convert pixels here?
    //      instead of during the paint process?
    printf("pal start: %d, pal count: %d, unkown: %d\n", info.start, info.count, info.unknown);

    GLuint tex = video_buffer.video_texture;
    int w      = video_buffer.render_w;
    int h      = video_buffer.render_h;

    blit_to_texture(tex, video_buffer.pxls, w, h);
    // swap buffers
    if (video_buffer.pxls == video_buffer.frnt_buffer) {
        video_buffer.pxls = video_buffer.back_buffer;
    } else {
        video_buffer.pxls = video_buffer.frnt_buffer;
    }
}

void parse_video_chunk(FILE* fileptr, chunkinfo info)
{
    uint8_t* chunk = (uint8_t*)calloc(1, info.size);
    fread(chunk, info.size, 1, fileptr);
    printf("chunk -- size: %d type: %d\n", info.size, info.type);

    int offset = 0;
    while (offset < info.size) {
        opcodeinfo op;
        memcpy(&op, &chunk[offset], sizeof(op));
        printf("op -- len: %d, type: %04x, ver: %d\n", op.size, op.type, op.version);

        offset += 4;
        parse_opcode(op, &chunk[offset]);
        offset += op.size;
    }
    printf("done processing video chunk\n");
    free(chunk);
}

void parse_decoding_map(uint8_t* buffer, int size)
{
    video_buffer.map_size   = size;
    video_buffer.map_stream = (uint8_t*)malloc(size);
    memcpy(video_buffer.map_stream, buffer, size);
}

int blockCopy_0x00(uint8_t* data_stream, uint8_t* dst_buff, int offset_x, int offset_y, bool blit)
{
    uint8_t* block_buff = video_buffer.block_buffer;
    int buff_pitch      = video_buffer.block_pitch;
    int frame_pitch     = video_buffer.pitch;

    //TODO: "current" should be the previous frame rendered
    //copy from "current" frame?
    uint8_t* current = NULL;
    uint8_t* next = video_buffer.pxls;
    if (next == video_buffer.frnt_buffer) {
        current = video_buffer.back_buffer;
    } else {
        current = video_buffer.frnt_buffer;
    }

    Rect src_rect = {
        .x = offset_x,
        .y = offset_y,
        .w = 8,
        .h = 8,
    };
    Rect buff_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };

    BlitSurface(current, src_rect, block_buff, buff_rect, frame_pitch, buff_pitch);

    if (blit) {
        BlitSurface(block_buff, buff_rect, dst_buff, buff_rect, buff_pitch, frame_pitch);
    }

    return 0;
}

int cornerCopy_0x02(uint8_t* data_stream, int x_offset, int y_offset, uint8_t* dst_buff, bool blit)
{
    uint8_t* block_buff = video_buffer.block_buffer;
    int buff_pitch      = video_buffer.block_pitch;
    int frame_pitch     = video_buffer.pitch;
    palette* pal        = video_buffer.pal;

    uint8_t B = data_stream[0];

    Rect src_rect = {
        .w = 8,
        .h = 8
    };
    Rect buff_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };

    // offset from current position
    if (B < 56) {
        src_rect.x = 8+(B%7);
        src_rect.y =    B/7;
    } else {
        src_rect.x = -14 + ((B-56) %29);
        src_rect.y =   8 + ((B-56) /29);
    }

    // offset from total
    // src_rect.x += x_offset;
    // src_rect.y += y_offset;

    //TODO: which frame is "current" and which is "new"?
    //copy from "new" frame?
    BlitSurface(video_buffer.pxls, src_rect, block_buff, buff_rect, frame_pitch, buff_pitch);
    if (blit) {
        BlitSurface(block_buff, buff_rect, dst_buff, buff_rect, buff_pitch, frame_pitch);
    }

    return 1;
}

int cornerCopy_0x03(uint8_t* data_stream, int x_offset, int y_offset, uint8_t* dst_buff, bool blit)
{
    uint8_t* block_buff = video_buffer.block_buffer;
    int buff_pitch      = video_buffer.block_pitch;
    int frame_pitch     = video_buffer.pitch;
    palette* pal        = video_buffer.pal;

    uint8_t B = data_stream[0];

    Rect buff_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };
    Rect src_rect = {
        .w = 8,
        .h = 8
    };

    // offset from current position
    if (B < 56) {
        src_rect.x = -(8+(B%7));
        src_rect.y = -   (B/7);
    } else {
        src_rect.x = -(-14 + ((B-56) %29));
        src_rect.y = -(  8 + ((B-56) /29));
    }
    // offset from total
    // src_rect.x += x_offset;
    // src_rect.y += y_offset;

    //TODO: which frame is "current" and which is "new"?
    //copy from "new" frame?
    BlitSurface(video_buffer.pxls, src_rect, block_buff, buff_rect, frame_pitch, buff_pitch);

    if (blit) {
        BlitSurface(block_buff, buff_rect, dst_buff, buff_rect, buff_pitch, frame_pitch);
    }

    return 1;
}

int symmetricCopy_0x04(uint8_t* data_stream, int x_offset, int y_offset, uint8_t* dst_buff, bool blit)
{
    uint8_t* block_buff = video_buffer.block_buffer;
    int buff_pitch      = video_buffer.block_pitch;
    int frame_pitch     = video_buffer.pitch;
    palette* pal        = video_buffer.pal;

#pragma pack(push,1)
    struct byte {
        uint8_t BL : 4;
        uint8_t BH : 4;
    } B;
#pragma pack(pop)
    memcpy(&B, data_stream, sizeof(B));

    // offset from current position
    Rect src_rect = {
        .x = -8 + B.BL,
        .y = -8 + B.BH,
        .w = 8,
        .h = 8
    };
    // offset from total
    // src_rect.x += x_offset;
    // src_rect.y += y_offset;


    //TODO: which frame is "current" and which is "new"?
    //copy from "current" frame?
    uint8_t* current = NULL;
    if (video_buffer.pxls == video_buffer.frnt_buffer) {
        current = video_buffer.back_buffer;
    } else {
        current = video_buffer.frnt_buffer;
    }
    Rect buff_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };
    BlitSurface(current, src_rect, block_buff, buff_rect, frame_pitch, buff_pitch);

    if (blit) {
        BlitSurface(block_buff, buff_rect, dst_buff, buff_rect, buff_pitch, frame_pitch);
    }

    return 1;
}

int symmetricCopy_0x05(uint8_t* data_stream, int x_offset, int y_offset, uint8_t* dst_buff, bool blit)
{
    uint8_t* block_buff = video_buffer.block_buffer;
    int buff_pitch      = video_buffer.block_pitch;
    int frame_pitch     = video_buffer.pitch;
    palette* pal        = video_buffer.pal;

    int8_t B[2] = {
        (int8_t)data_stream[0],
        (int8_t)data_stream[1]
    };

    // offset from current position
    Rect src_rect = {
        .x = B[0],
        .y = B[1],
        .w = 8,
        .h = 8
    };
    // offset from total
    // src_rect.x += x_offset;
    // src_rect.y += y_offset;

    uint8_t* current = NULL;
    if (video_buffer.pxls == video_buffer.frnt_buffer) {
        current = video_buffer.back_buffer;
    } else {
        current = video_buffer.frnt_buffer;
    }

    //TODO: which frame is "current" and which is "new"?
    //copy from "current" frame?
    Rect buff_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };
    BlitSurface(current, src_rect, block_buff, buff_rect, frame_pitch, buff_pitch);
    if (blit) {
        BlitSurface(block_buff, buff_rect, dst_buff, buff_rect, buff_pitch, frame_pitch);
    }

    return 2;
}

int pattern_0x07(uint8_t* data_stream, video* video, uint8_t* dst_buff, bool blit)
{
    int offset = 0;

    uint8_t* block_buff = video_buffer.block_buffer;
    int buff_pitch      = video_buffer.block_pitch;
    int dst_pitch       = video_buffer.pitch;
    palette* pal        = video_buffer.pal;

    uint8_t P[2] = {
        data_stream[0],
        data_stream[1],
    };
    uint8_t B[8];

    // printf("pattern 0x07: P[0]: %d P[1]: %d\n", P[0], P[1]);

    if (P[0] <= P[1]) {
        offset = 10;
        for (int i = 0; i < 8; i++)
        {
            B[i] = data_stream[i+2];
        }

        uint8_t mask = 128;
        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 8; x++)
            {
                bool pal_index = B[y] & (mask >> x);

                int pos = y*buff_pitch + x*3;
                palette color = pal[P[pal_index]];

                // color.r = 255;
                // color.g = 255;
                // color.b = 255;

                block_buff[pos + 0] = color.r;
                block_buff[pos + 1] = color.g;
                block_buff[pos + 2] = color.b;
            }
        }

    } else {
        offset = 4;
        B[0] = data_stream[2];
        B[1] = data_stream[3];
        // 2x2 blocks
        //-------------------------
        // 22 22 22 22 22 22 22 22 ; f == 1 0 1 1
        // 22 22 22 22 22 22 22 22 ; 
        // 22 22 22 22 22 22 22 22 ; f == 1 1 1 1
        // 22 22 22 22 22 22 22 22 ; 
        // 22 22 11 11 11 11 11 11 ; 8 == 1 0 0 0
        // 22 22 11 11 11 11 11 11 ; 
        // 11 11 11 11 11 11 22 22 ; 1 == 0 0 0 1
        // 11 11 11 11 11 11 22 22 ;

        int byte_index = 0;
        uint8_t mask = 0x80;
        uint8_t mask_offset = 0;
        for (int y = 0; y < 8; y+=2)
        {
            if (y >= 4) {
                byte_index = 1;
            }
            for (int x = 0; x < 8; x+=2)
            {
                bool pal_index = B[byte_index] & (mask >> mask_offset++);
                if (mask_offset >= 8) {
                    mask_offset = 0;
                }
                palette color = pal[P[pal_index]];

                Rect buff_rect = {
                    .x = x,
                    .y = y,
                    .w = 2,
                    .h = 2,
                };

                // color.r = 255;
                // color.g = 255;
                // color.b = 255;

                PaintSurface(block_buff, buff_pitch, buff_rect, color);
            }
        }
    }

    Rect src_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };

    if (blit) {
        BlitSurface(block_buff, src_rect, dst_buff, src_rect, buff_pitch, dst_pitch);
    }

    return offset;
}

int pattern_0x08(uint8_t* data_stream, video* video, uint8_t* dst_buff, bool blit)
{
    int offset = 0;

    uint8_t* buffer = video_buffer.block_buffer;
    int buff_pitch  = video_buffer.block_pitch;
    int dst_pitch   = video_buffer.pitch;
    palette* pal    = video_buffer.pal;

#pragma pack(push,1)
    struct quadrant {
        uint8_t P0; uint8_t P1; uint8_t B0; uint8_t B1;
    };
    struct halfpart {
        uint8_t P0;
        uint8_t P1;
        uint8_t B0;
        uint8_t B1;
        uint8_t B2;
        uint8_t B3;
    };
    union parse_video
    {
        quadrant quad[4];
        halfpart half[2];
    };
#pragma pack(pop)

    parse_video block;

    memcpy(&block, data_stream, sizeof(block));

    int quad_buff_pitch = 4*3;
    uint8_t quad_buffer[16*3] = {
        255,255,255,  255,255,255,  255,255,255,  255,255,255,
        255,255,255,  255,255,255,  255,255,255,  255,255,255,
        255,255,255,  255,255,255,  255,255,255,  255,255,255,
        255,255,255,  255,255,255,  255,255,255,  255,255,255,
    };
    uint8_t half_buffer[32*3] = {
        255,255,255,  255,255,255,  255,255,255,  255,255,255,
        255,255,255,  255,255,255,  255,255,255,  255,255,255,
        255,255,255,  255,255,255,  255,255,255,  255,255,255,
        255,255,255,  255,255,255,  255,255,255,  255,255,255,

        255,255,255,  255,255,255,  255,255,255,  255,255,255,
        255,255,255,  255,255,255,  255,255,255,  255,255,255,
        255,255,255,  255,255,255,  255,255,255,  255,255,255,
        255,255,255,  255,255,255,  255,255,255,  255,255,255,
    };

    //TODO: this needs to go     TL=>BL=>TR=>BR
    //      currently it's going TL=>TR=>BL=>BR
    //if P0 <= P1 we get all 4 corners
    uint8_t mask_offset = 0;
    if (block.quad[0].P0 <= block.quad[0].P1) {
        offset = 16;
        for (int q = 0; q < 4; q++)
        {
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
                        indx = block.quad[q].B0 & mask;
                    } else {
                        indx = block.quad[q].B1 & mask;
                    }
                    palette color = indx ? pal[block.quad[q].P0] : pal[block.quad[q].P1];

                    quad_buffer[y*quad_buff_pitch + x*3 +0] = color.r;
                    quad_buffer[y*quad_buff_pitch + x*3 +1] = color.g;
                    quad_buffer[y*quad_buff_pitch + x*3 +2] = color.b;
                }
            }
            Rect src_rect = {
                .x = 0,
                .y = 0,
                .w = 4,
                .h = 4,
            };
            Rect dst_rect = {
                .x = 0,
                .y = 0,
                .w = 4,
                .h = 4,
            };
            // q==0 => x = 0
            //         y = 0
            // q==1 => x = 4
            //         y = 0
            // q==2 => x = 0
            //         y = 4
            // q==3 => x = 4
            //         y = 4
            switch (q)
            {
            case 1:
                dst_rect.x = 4;
                break;
            case 2:
                dst_rect.y = 4;
                break;
            case 3:
                dst_rect.x = 4;
                dst_rect.y = 4;
                break;
            default:
                break;
            }
            if (blit) {
                BlitSurface(&quad_buffer[0], src_rect, dst_buff, dst_rect, quad_buff_pitch, video->pitch);
            }
        }
    } else {
        offset = 12;
        if (block.half[1].P0 <= block.half[1].P1) {
            //index 0 => left half
            //index 1 => right half
            //     Now, for a horizontally split block:

            //    22 00       | 11 66
            //    01 37 f7 31 | 8c e6 73 31

            //    22 22 22 22 | 66 11 11 11
            //    22 22 22 00 | 66 66 11 11
            //    22 22 00 00 | 66 66 66 11
            //    22 00 00 00 | 11 66 66 11
            //    00 00 00 00 | 11 66 66 66
            //    22 00 00 00 | 11 11 66 66
            //    22 22 00 00 | 11 11 66 66
            //    22 22 22 00 | 11 11 11 66

            int half_buff_pitch = 4*3;
            for (int h = 0; h < 2; h++)
            {
                for (int y = 0; y < 8; y++)
                {
                    for (int x = 0; x < 4; x++)
                    {
                        uint8_t mask = 128 >> mask_offset++;
                        if (mask_offset >= 8) {
                            mask_offset = 0;
                        }
                        bool which;
                        if (y < 2) {
                            which = block.half[h].B0 & mask;
                        } else
                        if (y >= 2 && y <= 3) {
                            which = block.half[h].B1 & mask;
                        } else
                        if (y >= 4 && y <= 5) {
                            which = block.half[h].B2 & mask;
                        } else
                        if (y >= 6 && y <= 7) {
                            which = block.half[h].B3 & mask;
                        }

                        palette color = which ? pal[block.half[h].P0] : pal[block.half[h].P1];

                        half_buffer[y*half_buff_pitch + x*3 + 0] = color.r;
                        half_buffer[y*half_buff_pitch + x*3 + 1] = color.g;
                        half_buffer[y*half_buff_pitch + x*3 + 2] = color.b;
                    }
                }
                Rect src_rect = {
                    .x = 0,
                    .y = 0,
                    .w = 4,
                    .h = 8,
                };
                Rect dst_rect = {
                    .x = 0,
                    .y = 0,
                    .w = 4,
                    .h = 8,
                };
                // h==0 => x = 0
                //         y = 0
                // h==1 => x = 4
                //         y = 0
                switch (h)
                {
                case 1:
                    dst_rect.x = 4;
                    break;

                default:
                    break;
                }
                if (blit) {
                    BlitSurface(&half_buffer[0], src_rect, dst_buff, dst_rect, half_buff_pitch, video->pitch);
                }
            }
        } else {
            //index 0 => top half
            //index 1 => bottom half
            //    Finally, for a vertically split block:
            //    22 00         66 11
            //    cc 66 33 19 | 18 24 42 81

            //    00 00 22 22 00 00 22 22
            //    22 00 00 22 22 00 00 22
            //    22 22 00 00 22 22 00 00
            //    22 22 22 00 00 22 22 00
            //    -----------------------
            //    66 66 66 11 11 66 66 66
            //    66 66 11 66 66 11 66 66
            //    66 11 66 66 66 66 11 66
            //    11 66 66 66 66 66 66 11
            int half_buff_pitch = 8*3;
            for (int h = 0; h < 2; h++)
            {
                for (int y = 0; y < 4; y++)
                {
                    for (int x = 0; x < 8; x++)
                    {
                        uint8_t mask = 128 >> mask_offset++;
                        if (mask_offset >= 8) {
                            mask_offset = 0;
                        }
                        bool which;
                        switch (y)
                        {
                        case 0:
                            which = block.half[h].B0 & mask;
                            break;
                        case 1:
                            which = block.half[h].B1 & mask;
                            break;
                        case 2:
                            which = block.half[h].B2 & mask;
                            break;
                        case 3:
                            which = block.half[h].B3 & mask;
                            break;
                        default:
                            break;
                        }

                        palette color = which ? pal[block.half[h].P0] : pal[block.half[h].P1];

                        half_buffer[y*half_buff_pitch + x*3 + 0] = color.r;
                        half_buffer[y*half_buff_pitch + x*3 + 1] = color.g;
                        half_buffer[y*half_buff_pitch + x*3 + 2] = color.b;
                    }
                }
                Rect src_rect = {
                    .x = 0,
                    .y = 0,
                    .w = 8,
                    .h = 4,
                };
                Rect dst_rect = {
                    // h==0 => x = 0
                    //         y = 0
                    // h==1 => x = 0
                    //         y = 4
                    .x = 0,
                    .y = 0,
                    .w = 8,
                    .h = 4,
                };
                switch (h)
                {
                case 1:
                    dst_rect.y = 4;
                    break;

                default:
                    break;
                }
                if (blit) {
                    BlitSurface(&half_buffer[0], src_rect, dst_buff, dst_rect, half_buff_pitch, video->pitch);
                }
            }
        }
    }

    return offset;
}

int pattern_0x09(uint8_t* data_stream, video* video, uint8_t* dst_buff, bool blit)
{
    int offset = 0;

    uint8_t* buffer = video_buffer.block_buffer;
    int buff_pitch  = video_buffer.block_pitch;
    int dst_pitch   = video_buffer.pitch;
    palette* pal    = video_buffer.pal;

    #pragma pack(push, 1)
    struct PXL_pattern {
        uint8_t P[4];
        uint8_t B[16];
    } pattern;
    #pragma pack(pop)
    memcpy(&pattern, data_stream, sizeof(pattern));

    uint8_t* P = pattern.P;
    uint8_t* B = pattern.B;
    uint8_t mask = 0xC0;
    int mask_offset = 0;
    int byte_index = 0;

    if (P[0] <= P[1] && P[2] <= P[3]) {
        offset = 20;
        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 8; x++)
            {
                uint8_t pal_index = B[byte_index] & mask >> mask_offset;

                switch (mask_offset)
                {
                case 0:
                    pal_index >>= 6;
                    break;
                case 2:
                    pal_index >>= 4;
                    break;
                case 4:
                    pal_index >>= 2;
                    break;
                case 6:
                    byte_index++;
                    if (byte_index >= 16) {
                        byte_index = 0;
                    }
                }
                palette color = pal[P[pal_index]];

                buffer[y*buff_pitch + x*3 + 0] = color.r;
                buffer[y*buff_pitch + x*3 + 1] = color.g;
                buffer[y*buff_pitch + x*3 + 2] = color.b;

                mask_offset += 2;
                if (mask_offset >= 8) {
                    mask_offset = 0;
                }
            }
        }
    }
    if (P[0] <= P[1] && P[2] > P[3]) {
        offset = 8;
        for (int y = 0; y < 8; y+=2)
        {
            for (int x = 0; x < 8; x+=2)
            {
                uint8_t pal_index = B[byte_index] & mask >> mask_offset;

                switch (mask_offset)
                {
                case 0:
                    pal_index >>= 6;
                    break;
                case 2:
                    pal_index >>= 4;
                    break;
                case 4:
                    pal_index >>= 2;
                    break;
                case 6:
                    byte_index++;
                    if (byte_index >= 16) {
                        byte_index = 0;
                    }
                }
                palette color = pal[P[pal_index]];

                Rect buff_rect ={
                    .x = x,
                    .y = y,
                    .w = 2,
                    .h = 2,
                };
                PaintSurface(&buffer[0], buff_pitch, buff_rect, color);

                mask_offset += 2;
                if (mask_offset >= 8) {
                    mask_offset = 0;
                }
            }
        }
    }
    if (P[0] > P[1] && P[2] <= P[3]) {
        offset = 12;
        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 8; x+=2)
            {
                uint8_t pal_index = B[byte_index] & mask >> mask_offset;

                switch (mask_offset)
                {
                case 0:
                    pal_index >>= 6;
                    break;
                case 2:
                    pal_index >>= 4;
                    break;
                case 4:
                    pal_index >>= 2;
                    break;
                case 6:
                    byte_index++;
                    if (byte_index >= 16) {
                        byte_index = 0;
                    }
                }
                palette color = pal[P[pal_index]];

                Rect buff_rect ={
                    .x = x,
                    .y = y,
                    .w = 2,
                    .h = 1,
                };
                PaintSurface(&buffer[0], buff_pitch, buff_rect, color);

                mask_offset += 2;
                if (mask_offset >= 8) {
                    mask_offset = 0;
                }
            }
        }
    }
    if (P[0] > P[1] && P[2] > P[3]) {
        offset = 12;
        for (int y = 0; y < 8; y+=2)
        {
            for (int x = 0; x < 8; x++)
            {
                uint8_t pal_index = B[byte_index] & mask >> mask_offset;

                switch (mask_offset)
                {
                case 0:
                    pal_index >>= 6;
                    break;
                case 2:
                    pal_index >>= 4;
                    break;
                case 4:
                    pal_index >>= 2;
                    break;
                case 6:
                    byte_index++;
                    if (byte_index >= 16) {
                        byte_index = 0;
                    }
                }
                palette color = pal[P[pal_index]];

                Rect buff_rect ={
                    .x = x,
                    .y = y,
                    .w = 1,
                    .h = 2,
                };
                PaintSurface(&buffer[0], buff_pitch, buff_rect, color);

                mask_offset += 2;
                if (mask_offset >= 8) {
                    mask_offset = 0;
                }
            }
        }
    }

    Rect src_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };
    if (blit) {
        BlitSurface(buffer, src_rect, dst_buff, src_rect, buff_pitch, dst_pitch);
    }
    return offset;
}

int pattern_0x0A(uint8_t* data_stream, video* video, uint8_t* dst_buff, bool blit)
{
    int offset = 0;

    uint8_t* buffer = video_buffer.block_buffer;
    int buff_pitch  = video_buffer.block_pitch;
    int dst_pitch   = video_buffer.pitch;
    palette* pal    = video_buffer.pal;

    #pragma pack(push, 1)
    struct pxls32 {
        uint8_t P[4];
        uint8_t B[4];
    };
    struct pxls24 {
        uint8_t P[4];
        uint8_t B[8];
    };
    union parse_video {
        pxls32 px32[4];
        pxls24 px24[2];
    } pattern;
    #pragma pack(pop)
    memcpy(&pattern, data_stream, sizeof(pattern));

    uint8_t mask = 0xC0;
    int mask_offset = 0;
    int byte_index = 0;
    if (data_stream[0] <= data_stream[1]) {
        offset = 32;

        for (int q = 0; q < 4; q++)
        {
            uint8_t* P = pattern.px32[q].P; //q==0 =>TL, 1=>BL
            uint8_t* B = pattern.px32[q].B; //q==2 =>TR, 3=>BR
            for (int y = 0; y < 4; y++)
            {
                for (int x = 0; x < 4; x++)
                {
                    uint8_t pal_index = B[byte_index] & mask >> mask_offset;
                    switch (mask_offset)
                    {
                    case 0:
                        pal_index >>= 6;
                        break;
                    case 2:
                        pal_index >>= 4;
                        break;
                    case 4:
                        pal_index >>= 2;
                        break;
                    case 6:
                        byte_index++;
                        if (byte_index >= 4) {
                            byte_index = 0;
                        }
                    }
                    palette color = pal[P[pal_index]];

                    int quad_offsety = 0;
                    int quad_offsetx = 0;
                    switch (q)
                    {
                    case 1:
                        quad_offsety = 4;
                        quad_offsetx = 0;
                        break;
                    case 2:
                        quad_offsety = 0;
                        quad_offsetx = 4;
                        break;
                    case 3:
                        quad_offsety = 4;
                        quad_offsetx = 4;
                        break;
                    }

                    buffer[(y + quad_offsety)*buff_pitch + (x + quad_offsetx)*3 + 0] = color.r;
                    buffer[(y + quad_offsety)*buff_pitch + (x + quad_offsetx)*3 + 1] = color.g;
                    buffer[(y + quad_offsety)*buff_pitch + (x + quad_offsetx)*3 + 2] = color.b;


                    mask_offset += 2;
                    if (mask_offset >= 8) {
                        mask_offset =  0;
                    }
                }
            }
        }
        Rect src_rect = {
            .x = 0,
            .y = 0,
            .w = 8,
            .h = 8,
        };
        if (blit) {
            BlitSurface(buffer, src_rect, dst_buff, src_rect, buff_pitch, dst_pitch);
        }
    } else {
    // if (data_stream[0] > data_stream[1]) {
        offset = 24;
        for (int half = 0; half < 2; half++)
        {
            uint8_t* P = pattern.px24[half].P;
            uint8_t* B = pattern.px24[half].B;
            int w;
            int h;
            if (pattern.px24[1].P[0] <= pattern.px24[1].P[1]) {
                //left and right half
                w = 4;
                h = 8;
            } else {
                //top and bottom half
                w = 8;
                h = 4;
            }

            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    uint8_t pal_index = B[byte_index] & mask >> mask_offset;
                    switch (mask_offset)
                    {
                    case 0:
                        pal_index >>= 6;
                        break;
                    case 2:
                        pal_index >>= 4;
                        break;
                    case 4:
                        pal_index >>= 2;
                        break;
                    case 6:
                        byte_index++;
                        if (byte_index >= 8) {
                            byte_index = 0;
                        }
                    }
                    palette color = pal[P[pal_index]];

                    int half_offsety = 0;
                    int half_offsetx = 0;
                    if (half == 1) {
                        if (w == 4) {
                            half_offsetx = 4;
                        } else {
                            half_offsety = 4;
                        }
                        break;
                    }

                    buffer[(y + half_offsety)*buff_pitch + (x + half_offsetx)*3 + 0] = color.r;
                    buffer[(y + half_offsety)*buff_pitch + (x + half_offsetx)*3 + 1] = color.g;
                    buffer[(y + half_offsety)*buff_pitch + (x + half_offsetx)*3 + 2] = color.b;

                    mask_offset += 2;
                    if (mask_offset >= 8) {
                        mask_offset =  0;
                    }
                }
            }
        }
        Rect src_rect = {
            .x = 0,
            .y = 0,
            .w = 8,
            .h = 8,
        };
        if (blit) {
            BlitSurface(buffer, src_rect, dst_buff, src_rect, buff_pitch, dst_pitch);
        }
    }
    return offset;
}

int raw_pixels_0x0B(uint8_t* data_stream, video* video_buffer, uint8_t* dst_buff, bool blit)
{
    uint8_t* buffer = video_buffer->block_buffer;
    int buff_pitch  = video_buffer->block_pitch;
    int dst_pitch   = video_buffer->pitch;

    uint8_t P[64];
    palette* pal = video_buffer->pal;

    memcpy(P, data_stream, sizeof(P));

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            palette color = pal[P[y*8 + x]];
            buffer[y*buff_pitch + x*3 + 0] = color.r;
            buffer[y*buff_pitch + x*3 + 1] = color.g;
            buffer[y*buff_pitch + x*3 + 2] = color.b;
        }
    }

    Rect src_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };

    if (blit) {
        BlitSurface(buffer, src_rect, dst_buff, src_rect, buff_pitch, dst_pitch);
    }

    return 64;
}

int raw_pixels_0x0C(uint8_t* data_stream, video* video_buffer, uint8_t* dst_buff, bool blit)
{
    uint8_t* buffer = video_buffer->block_buffer;
    int buff_pitch  = video_buffer->block_pitch;
    int dst_pitch   = video_buffer->pitch;

    uint8_t P[16];
    memcpy(P, data_stream, sizeof(P));

    int byte_index = 0;
    palette* pal = video_buffer->pal;
    for (int y = 0; y < 8; y+=2) {
        for (int x = 0; x < 8; x+=2) {
            palette color = pal[P[byte_index++]];
            Rect dst_rect = {
                .x = x,
                .y = y,
                .w = 2,
                .h = 2,
            };
            PaintSurface(buffer, buff_pitch, dst_rect, color);
        }
    }

    Rect src_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };
    if (blit) {
        BlitSurface(buffer, src_rect, dst_buff, src_rect, buff_pitch, dst_pitch);
    }

    return 16;
}

int raw_pixels_0x0D(uint8_t* data_stream, video*video_buffer, uint8_t* dst_buff, bool blit)
{
    uint8_t* buffer = video_buffer->block_buffer;
    int buff_pitch  = video_buffer->block_pitch;
    int dst_pitch   = video_buffer->pitch;

    uint8_t P[4];
    memcpy(P, data_stream, sizeof(P));

    palette* pal = video_buffer->pal;

    int byte_index = 0;
    for (int y = 0; y < 8; y+=4) {
        for (int x = 0; x < 8; x+=4) {
            palette color = pal[P[byte_index++]];

            Rect dst_rect = {
                .x = x,
                .y = y,
                .w = 4,
                .h = 4,
            };
            PaintSurface(buffer, buff_pitch, dst_rect, color);
        }
    }
    Rect src_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };
    if (blit) {
        BlitSurface(buffer, src_rect, dst_buff, src_rect, buff_pitch, dst_pitch);
    }

    return 4;
}

//set the whole 8x8 pixels a solid color
int solid_frame_0x0E(uint8_t* data_stream, video* video_buffer, uint8_t* dst_buff, bool paint)
{
    uint8_t pal_index = data_stream[0];
    Rect dst_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };

    palette color = video_buffer->pal[pal_index];
    int pitch = video_buffer->pitch;
    printf("pal_index: %d  RGB %d%d%d\n", pal_index, color.r, color.g, color.b);

    //TODO: paint a buffer first, then blit
    if (paint) {
        PaintSurface(dst_buff, pitch, dst_rect, color);
    }

    return 1;
}

int dithered_0x0F(uint8_t* data_stream, video*video_buffer, uint8_t* dst_buff, bool blit)
{
    uint8_t* buffer = video_buffer->block_buffer;
    int buff_pitch = 8*3;
    uint8_t P[2] = {
        data_stream[0],
        data_stream[1]
    };

    palette* pal = video_buffer->pal;
    palette color1 = pal[P[0]];
    palette color2 = pal[P[1]];

    for (int i = 0; i < 64; i+=2)
    {
        // buffer[i*3] = color1.color;
        buffer[i*3 +0] = color1.r;
        buffer[i*3 +1] = color1.g;
        buffer[i*3 +2] = color1.b;
    }
    for (int i = 1; i < 64; i+=2)
    {
        // buffer[i*3] = color2.color;
        buffer[i*3 +0] = color2.r;
        buffer[i*3 +1] = color2.g;
        buffer[i*3 +2] = color2.b;
    }

    Rect dst_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8
    };
    if (blit) {
        BlitSurface(buffer, dst_rect, dst_buff, dst_rect, buff_pitch, video_buffer->pitch);
    }

    return 2;
}

int parse_video_encode(uint8_t op, uint8_t* data_stream, uint8_t* frame_buffer, int x_offset, int y_offset)
{
    video_buffer.encode_type[op]++;

    bool* allow_blit = video_buffer.allow_blit;

    int offset = 0;
    switch (op)
    {
    case 0x00:
        offset = blockCopy_0x00(data_stream, frame_buffer, x_offset, y_offset, allow_blit[op]);
        //offset = 0;
        break;
    case 0x01:
        offset = 0;
        break;
    case 0x02:
        offset = cornerCopy_0x02(data_stream, x_offset, y_offset, frame_buffer, allow_blit[op]);
        //offset = 1;
        break;
    case 0x03:
        offset = cornerCopy_0x03(data_stream,x_offset, y_offset, frame_buffer, allow_blit[op]);
        //offset = 1;
        break;
    case 0x04:
        offset = symmetricCopy_0x04(data_stream,x_offset, y_offset, frame_buffer, allow_blit[op]);
        //offset = 1;
        break;
    case 0x05:
        offset = symmetricCopy_0x05(data_stream,x_offset, y_offset, frame_buffer, allow_blit[op]);
        //offset = 2;
        break;
    case 0x06:
        offset = 0;
        //no opcode 0x06 found so far
        break;
    case 0x07:
        offset = pattern_0x07(data_stream, &video_buffer, frame_buffer, allow_blit[op]);
        //offset = 10;
        //  or
        //offset = 4;
        break;
    case 0x08:
        offset = pattern_0x08(data_stream, &video_buffer, frame_buffer, allow_blit[op]);
        //offset = 16;
        //  or
        //offset = 12;
        break;
    case 0x09:
        offset = pattern_0x09(data_stream, &video_buffer, frame_buffer, allow_blit[op]);
        //offset = 20;
        //offset = 8;
        //offset = 12;
        //offset = 12;
        break;
    case 0x0A:
        offset = pattern_0x0A(data_stream, &video_buffer, frame_buffer, allow_blit[op]);
        //offset = 32;
        //offset = 24;
        break;
    case 0x0B:
        offset = raw_pixels_0x0B(data_stream, &video_buffer, frame_buffer, allow_blit[op]);
        //offset = 64;
        break;
    case 0x0C:
        offset = raw_pixels_0x0C(data_stream, &video_buffer, frame_buffer, allow_blit[op]);
        //offset = 16;
        break;
    case 0x0D:
        offset = raw_pixels_0x0D(data_stream, &video_buffer, frame_buffer, allow_blit[op]);
        //offset = 4;
        break;
    case 0x0E:
        offset = solid_frame_0x0E(data_stream, &video_buffer, frame_buffer, allow_blit[op]);
        //offset = 1;
        break;
    case 0x0F:
        offset = dithered_0x0F(data_stream, &video_buffer, frame_buffer, allow_blit[op]);
        //offset = 2;
        break;

    default:
        break;
    }

    return offset;
}

void parse_video_data(uint8_t* buffer)
{
    uint8_t* map_stream = video_buffer.map_stream;
    uint8_t* data_stream = buffer;

    uint8_t* next_frame = video_buffer.pxls;

    uint8_t mask    = 0x0F;
    int data_offset = 0;
    int frame_pitch = video_buffer.pitch;
    int x_offset    = 0;
    int y_offset    = 0;

    for (int i = 0; i < video_buffer.map_size*2; i++)
    {
        mask = ~mask;
        uint8_t enc = map_stream[i/2] & mask;
        if (mask == 0xF0) {
            enc >>= 4;
        }

        // printf("enc: %0x  data_offset: %d\n", enc, data_offset);
        data_offset += parse_video_encode(
            enc,
            &data_stream[data_offset],
            &next_frame[y_offset*frame_pitch + x_offset],
            x_offset,
            y_offset
        );

        x_offset += 8*3;
        if (x_offset >= video_buffer.pitch) {
            x_offset  = 0;
            y_offset += 8;
        }
    }
}