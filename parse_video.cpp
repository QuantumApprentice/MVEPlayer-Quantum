#include "parse_video.h"
#include "parse_opcodes.h"
#include "MiniSDL.h"
#include "io_OpenGL.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

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
#ifdef DEBUG
    printf("pretty sure these are window dimensions - w: %d, h: %d, f: %04x\n", initinfo.w, initinfo.h, initinfo.f);
#endif
    video_buffer.window_w = initinfo.w;
    video_buffer.window_h = initinfo.h;
}

void init_video_buffer(uint8_t* buffer, uint8_t version)
{
    int buff_size = 0;
    int block_w   = 0;
    int block_h   = 0;
    switch (version)
    {
    case 0:{
        videoinit_buffer_v0 vbuff0;
        memcpy(&vbuff0, buffer, sizeof(vbuff0));
        buff_size = 2* vbuff0.w * vbuff0.h;
        block_w = vbuff0.w;
        block_h = vbuff0.h;
        break;}
    case 1:{
        videoinit_buffer_v1 vbuff1;
        memcpy(&vbuff1, buffer, sizeof(vbuff1));
        buff_size = 2* vbuff1.w * vbuff1.h * vbuff1.count;
        block_w = vbuff1.w;
        block_h = vbuff1.h;
        break;}
    case 2:{
        videoinit_buffer_v2 vbuff2;
        memcpy(&vbuff2, buffer, sizeof(vbuff2));
        buff_size = 2* vbuff2.w * vbuff2.h * vbuff2.count;
        block_w = vbuff2.w;
        block_h = vbuff2.h;
#ifdef DEBUG
        printf("vbuff w: %d, vbuff h, %d, vbuff count: %d\n", vbuff2.w, vbuff2.h, vbuff2.count);
#endif
        break;}
    default:
        printf("Undocumented version for init_video_buffer() found: %d\n", version);
        break;
    }
#ifdef DEBUG
        printf("video buffer size: %d\n", buff_size);
#endif

    video_buffer.block_w   = block_w;
    video_buffer.block_h   = block_h;
    video_buffer.buff_size = buff_size; //what is this buff_size actually for?
    video_buffer.render_w  = block_w*8;
    video_buffer.render_h  = block_h*8;
    video_buffer.pitch     = block_w * 8 * 3;

    if (video_buffer.frnt_buffer) {
        free(video_buffer.frnt_buffer);
        video_buffer.frnt_buffer = NULL;
    }
    if (video_buffer.back_buffer) {
        free(video_buffer.back_buffer);
        video_buffer.back_buffer = NULL;
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

#ifdef DEBUG
    printf("pal start: %d, pal count: %d\n", pal_info.start, pal_info.count);
#endif
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
    for (int i = 0; i < pal_info.count; i++)
    {   //shift to get the palette color correct
        video_buffer.pal[pal_info.start + i].r = buffer[i*3 + 4 + 0] << 2;
        video_buffer.pal[pal_info.start + i].g = buffer[i*3 + 4 + 1] << 2;
        video_buffer.pal[pal_info.start + i].b = buffer[i*3 + 4 + 2] << 2;
    }

#ifdef DEBUG
    printf("palette colors:\n");
    for (int i = 0; i < pal_info.count + pal_info.start; i++)
    {
        printf("i:%d r:%d g:%d b:%d\n", i, video_buffer.pal[i].r, video_buffer.pal[i].g, video_buffer.pal[i].b);
    }
#endif
}

void init_video(uint8_t* chunk, chunkinfo info)
{
    int offset = 0;
    while (offset < info.size)
    {
        opcodeinfo op;
        memcpy(&op, &chunk[offset], sizeof(op));
        // printf("op -- len: %d, type: 0x%02X, ver: %d\n", op.size, op.type, op.version);

        offset += 4;
        parse_opcode(op, &chunk[offset]);
        offset += op.size;
    }
}

void create_timer(uint8_t* buffer)
{
    memcpy(&video_buffer.timer, buffer, sizeof(video_buffer.timer));

#ifdef DEBUG
    printf("frame rate: %d, subdivision: %d\n", video_buffer.timer.rate, video_buffer.timer.subdivision);
#endif
}

void send_buffer_to_display(uint8_t* buffer)
{
    video_buffer.frame_count++;
    video_buffer.v_chunk_per_frame = video_buffer.v_chunk_cnt_total - video_buffer.v_chunk_cnt_last;
    video_buffer.v_chunk_cnt_last  = video_buffer.v_chunk_cnt_total;
    struct pal_info {
        uint16_t start;
        uint16_t count;
        uint16_t unknown;
    } info;

    memcpy(&info, buffer, sizeof(info));

    //TODO: should I use palette to convert pixels here?
    //      instead of during the paint process?
    // printf("pal start: %d, pal count: %d, unkown: %d\n", info.start, info.count, info.unknown);

    GLuint tex = video_buffer.video_texture;
    int w      = video_buffer.render_w;
    int h      = video_buffer.render_h;

    blit_to_texture(tex, video_buffer.pxls, w, h);
}

bool parse_video_chunk(uint8_t* chunk, chunkinfo info)
{
    int offset = 0;
    bool render_frame = false;
    while (offset < info.size) {
        opcodeinfo op;
        memcpy(&op, &chunk[offset], sizeof(op));
        // printf("op -- len: %d, type: %04x, ver: %d\n", op.size, op.type, op.version);

        offset += 4;

        parse_opcode(op, &chunk[offset]);
        offset += op.size;

        if (op.type == 0x07) {
            render_frame = true;
        }
    }
    printf("done processing video chunk# %d\n", video_buffer.v_chunk_cnt_total++);

    return render_frame;
}

void parse_decoding_map(uint8_t* buffer, int size)
{
    video_buffer.map_size   = size;
    video_buffer.map_stream = (uint8_t*)malloc(size);
    memcpy(video_buffer.map_stream, buffer, size);
}

//TODO: delete data_stream (not used)
//offset = 0;
int blockCopy_0x00(uint8_t* data_stream, uint8_t* dst_buff, int offset_x, int offset_y, bool blit, bool mark)
{
    uint8_t* block_buff = video_buffer.block_buffer;
    int buff_pitch      = video_buffer.block_pitch;
    int frame_pitch     = video_buffer.pitch;

    //copy from "current" frame
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

    BlitSurface(current, src_rect, frame_pitch, block_buff, buff_rect, buff_pitch);
    if (mark) {
        PaintSurface(block_buff, buff_pitch, buff_rect, {255, 255, 255});
    }
    if (blit) {
        BlitSurface(block_buff, buff_rect, buff_pitch, dst_buff, buff_rect, frame_pitch);
    }

    return 0;
}

//offset = 1;
int cornerCopy_0x02(uint8_t* data_stream, int x_offset, int y_offset, uint8_t* dst_buff, bool blit, bool mark)
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

    // "new" frame is dst_buff
    // copy from "new" frame
    BlitSurface(dst_buff, src_rect, frame_pitch, block_buff, buff_rect, buff_pitch);
    if (mark) {
        PaintSurface(block_buff, buff_pitch, buff_rect, {255, 255, 255});
    }
    if (blit) {
        BlitSurface(block_buff, buff_rect, buff_pitch, dst_buff, buff_rect, frame_pitch);
    }

    return 1;
}

//offset = 1;
int cornerCopy_0x03(uint8_t* data_stream, int x_offset, int y_offset, uint8_t* dst_buff, bool blit, bool mark)
{
    uint8_t* block_buff = video_buffer.block_buffer;
    int buff_pitch      = video_buffer.block_pitch;
    int frame_pitch     = video_buffer.pitch;

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

    // "new" frame is video_buffer.pxls
    //copy from "new" frame
    BlitSurface(dst_buff, src_rect, frame_pitch, block_buff, buff_rect, buff_pitch);
    if (mark) {
        PaintSurface(block_buff, buff_pitch, buff_rect, {255, 255, 255});
    }
    if (blit) {
        BlitSurface(block_buff, buff_rect, buff_pitch, dst_buff, buff_rect, frame_pitch);
    }

    return 1;
}

//offset = 1;
int symmetricCopy_0x04(uint8_t* data_stream, int x_offset, int y_offset, uint8_t* dst_buff, bool blit, bool mark)
{
    uint8_t* block_buff = video_buffer.block_buffer;
    int block_pitch     = video_buffer.block_pitch;
    int frame_pitch     = video_buffer.pitch;

    //TODO: clean this up
    //      probably not use the bitfields (even though they do work)
#pragma pack(push,1)
    struct byte {
        uint8_t BL : 4;     //bitfield - only read the number of bits indicated?
        uint8_t BH : 4;     //
    } B;
#pragma pack(pop)
    // uint8_t B;
    memcpy(&B, data_stream, sizeof(B));

    // offset from current position
    Rect src_rect = {
        .x = -8 + B.BL,
        .y = -8 + B.BH,
        // .x = -8 + (B & 0xF),
        // .y = -8 + (B >> 4),
        .w = 8,
        .h = 8
    };
    // offset from total
    src_rect.x += x_offset;
    src_rect.y += y_offset;


    //copy from "current" frame
    uint8_t* current = NULL;
    if (video_buffer.pxls == video_buffer.frnt_buffer) {
        current = video_buffer.back_buffer;
    } else {
        current = video_buffer.frnt_buffer;
    }

    Rect block_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };
    BlitSurface(current, src_rect, frame_pitch, block_buff, block_rect, block_pitch);
    if (mark) {
        PaintSurface(block_buff, block_pitch, block_rect, {255, 255, 255});
    }
    if (blit) {
        BlitSurface(block_buff, block_rect, block_pitch, dst_buff, block_rect, frame_pitch);
    }

    return 1;
}

//offset = 2;
int symmetricCopy_0x05(uint8_t* data_stream, int x_offset, int y_offset, uint8_t* dst_buff, bool blit, bool mark)
{
    uint8_t* block_buff = video_buffer.block_buffer;
    int buff_pitch      = video_buffer.block_pitch;
    int frame_pitch     = video_buffer.pitch;

    int8_t B[2] = {
        (int8_t)data_stream[0],
        (int8_t)data_stream[1]
    };

    // offset from current position
    Rect src_rect = {
        .x = B[0] + x_offset,
        .y = B[1] + y_offset,
        .w = 8,
        .h = 8
    };

    uint8_t* current = NULL;
    if (video_buffer.pxls == video_buffer.frnt_buffer) {
        current = video_buffer.back_buffer;
    } else {
        current = video_buffer.frnt_buffer;
    }

    //copy from "current" frame
    Rect buff_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };
    BlitSurface(current, src_rect, frame_pitch, block_buff, buff_rect, buff_pitch);
    if (mark) {
        PaintSurface(block_buff, buff_pitch, buff_rect, {255, 255, 255});
    }
    if (blit) {
        BlitSurface(block_buff, buff_rect, buff_pitch, dst_buff, buff_rect, frame_pitch);
    }

    return 2;
}

//offset = 10 or 4;
int pattern_0x07(uint8_t* data_stream, video* video, uint8_t* dst_buff, bool blit, bool mark)
{
    int offset = 0;

    uint8_t* block_buff = video_buffer.block_buffer;
    int block_pitch     = video_buffer.block_pitch;
    int frame_pitch     = video_buffer.pitch;
    palette* pal        = video_buffer.pal;

    uint8_t P[2] = {
        data_stream[0],
        data_stream[1],
    };
    uint8_t B[8];

    if (P[0] <= P[1]) {
        offset = 10;
        memcpy(B, &data_stream[2], sizeof(B));

        uint8_t mask = 1;
        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 8; x++)
            {
                bool P_index = B[y] & (mask << x);
                palette color = pal[P[P_index]];

                int pos = y*block_pitch + x*3;
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
        uint8_t mask = 1;
        uint8_t mask_offset = 0;
        for (int y = 0; y < 8; y+=2)
        {
            if (y >= 4) {
                byte_index = 1;
            }
            for (int x = 0; x < 8; x+=2)
            {
                bool pal_index = B[byte_index] & (mask << mask_offset++);
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

                PaintSurface(block_buff, block_pitch, buff_rect, color);
            }
        }
    }

    Rect block_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };
    if (mark) {
        PaintSurface(block_buff, block_pitch, block_rect, {255, 255, 255});
    }
    if (blit) {
        BlitSurface(block_buff, block_rect, block_pitch, dst_buff, block_rect, frame_pitch);
    }

    return offset;
}

//offset = 16 or 12;
int pattern_0x08(uint8_t* data_stream, uint8_t* dst_buff, bool blit, bool mark)
{
    int offset = 0;

    uint8_t* block_buff = video_buffer.block_buffer;
    int block_pitch     = video_buffer.block_pitch;
    int frame_pitch     = video_buffer.pitch;
    palette* pal        = video_buffer.pal;

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
    union half_quads
    {
        quadrant quad[4];   //16 bytes
        halfpart half[2];   //12 bytes
    };
#pragma pack(pop)

    half_quads block;
    memcpy(&block, data_stream, sizeof(block));

    //paints in this order     TL=>BL=>TR=>BR
    //if P0 <= P1 we get all 4 corners
    uint8_t mask_offset = 0;
    if (block.quad[0].P0 <= block.quad[0].P1) {
        offset = 16;
        for (int q = 0; q < 4; q++)
        {
            Rect start = {
                .x = 0,             // q==0 => x = 0
                .y = 0,             //         y = 0
                .w = 4,
                .h = 4,
            };
            switch (q)
            {
            case 1:
                start.y = 4;        // q==1 => x = 0
                // start.h = 8;        //         y = 4
                break;
            case 2:
                start.x = 4;        // q==2 => x = 4
                // start.w = 8;        //         y = 0
                break;
            case 3:
                start.x = 4;        // q==3 => x = 4
                start.y = 4;        //         y = 4
                // start.w = 8;
                // start.h = 8;
                break;
            }

            for (int y = start.y; y < start.y+start.h; y++) {
                for (int x = start.x; x < start.x+start.w; x++) {
                    uint8_t mask = 1 << mask_offset++;
                    if (mask_offset >= 8) {
                        mask_offset = 0;
                    }
                    bool indx;
                    if (y < start.y+start.h-2) {
                        indx = block.quad[q].B0 & mask;
                    } else {
                        indx = block.quad[q].B1 & mask;
                    }
                    palette color = (indx == 0) ? pal[block.quad[q].P0] : pal[block.quad[q].P1];

                    block_buff[y*block_pitch + x*3 +0] = color.r;
                    block_buff[y*block_pitch + x*3 +1] = color.g;
                    block_buff[y*block_pitch + x*3 +2] = color.b;
                }
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

            for (int h = 0; h < 2; h++)
            {
                Rect start = {
                    .x = 0,         // h==0 => x = 0
                    .y = 0,         //         y = 0
                    .w = 4,
                    .h = 8,
                };
                if (h == 1) {
                    start.x = 4;    // h==1 => x = 4
                    start.w = 8;    //         y = 0
                }

                // Left & Right half
                for (int y = 0; y < 8; y++) {
                    for (int x = start.x; x < start.w; x++) {
                        uint8_t mask = 1 << mask_offset++;
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

                        palette color = (which == 0) ? pal[block.half[h].P0] : pal[block.half[h].P1];

                        block_buff[y*block_pitch + x*3 + 0] = color.r;
                        block_buff[y*block_pitch + x*3 + 1] = color.g;
                        block_buff[y*block_pitch + x*3 + 2] = color.b;
                    }
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
            for (int h = 0; h < 2; h++)
            {
                Rect start = {
                    .x = 0,         // h==0 => x = 0
                    .y = 0,         //         y = 0
                    .w = 8,
                    .h = 4,
                };
                if (h == 1) {
                    start.y = 4;    // h==1 => x = 0
                    start.h = 8;    //         y = 4
                }

                //Top & Bottom half
                for (int y = start.y; y < start.h; y++) {
                    for (int x = 0; x < 8; x++) {
                        uint8_t mask = 1 << mask_offset++;
                        if (mask_offset >= 8) {
                            mask_offset = 0;
                        }

                        uint8_t which;
                        switch (y&0x3)
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
                        }

                        palette color = (which == 0) ? pal[block.half[h].P0] : pal[block.half[h].P1];

                        block_buff[y*block_pitch + x*3 + 0] = color.r;
                        block_buff[y*block_pitch + x*3 + 1] = color.g;
                        block_buff[y*block_pitch + x*3 + 2] = color.b;
                    }
                }
            }
        }
    }
    Rect block_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };
    Rect dst_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };

    if (mark) {
        PaintSurface(block_buff, block_pitch, block_rect, {255, 255, 255});
    }
    if (blit) {
        BlitSurface(block_buff, block_rect, block_pitch, dst_buff, dst_rect, frame_pitch);
    }
    return offset;
}

//offset = 20 or 8 or 12 or 12;
int pattern_0x09(uint8_t* data_stream, video* video, uint8_t* dst_buff, bool blit, bool mark)
{
    int offset = 0;

    uint8_t* block_buff = video_buffer.block_buffer;
    int block_pitch     = video_buffer.block_pitch;
    int frame_pitch     = video_buffer.pitch;
    palette* pal        = video_buffer.pal;

    #pragma pack(push, 1)
    struct PXL_pattern {
        uint8_t P[4];
        uint8_t B[16];
    } pattern;
    #pragma pack(pop)
    memcpy(&pattern, data_stream, sizeof(pattern));

    uint8_t* P = pattern.P;
    uint8_t* B = pattern.B;
    uint8_t mask = 0x03;
    int mask_offset = 0;
    int byte_index = 0;

    if (P[0] <= P[1] && P[2] <= P[3]) {
        //patterned pixels
        offset = 20;
        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 8; x++)
            {
                uint8_t pal_index = B[byte_index] & (mask << mask_offset);

                switch (mask_offset)
                {
                case 2:
                    pal_index >>= 2;
                    break;
                case 4:
                    pal_index >>= 4;
                    break;
                case 6:
                    pal_index >>= 6;
                    byte_index++;
                    if (byte_index >= 16) {
                        byte_index = 0;
                    }
                }
                palette color = pal[P[pal_index]];

                block_buff[y*block_pitch + x*3 + 0] = color.r;
                block_buff[y*block_pitch + x*3 + 1] = color.g;
                block_buff[y*block_pitch + x*3 + 2] = color.b;

                mask_offset += 2;
                if (mask_offset >= 8) {
                    mask_offset = 0;
                }
            }
        }
    }
    if (P[0] <= P[1] && P[2] > P[3]) {
        //2x2 pixel pattern
        offset = 8;
        for (int y = 0; y < 8; y+=2)
        {
            for (int x = 0; x < 8; x+=2)
            {
                uint8_t pal_index = B[byte_index] & (mask << mask_offset);

                switch (mask_offset)
                {
                case 2:
                    pal_index >>= 2;
                    break;
                case 4:
                    pal_index >>= 4;
                    break;
                case 6:
                    pal_index >>= 6;
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
                PaintSurface(block_buff, block_pitch, buff_rect, color);

                mask_offset += 2;
                if (mask_offset >= 8) {
                    mask_offset = 0;
                }
            }
        }
    }
    if (P[0] > P[1] && P[2] <= P[3]) {
        //2x1 pixel pattern
        offset = 12;
        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 8; x+=2)
            {
                uint8_t pal_index = B[byte_index] & (mask << mask_offset);

                switch (mask_offset)
                {
                case 2:
                    pal_index >>= 2;
                    break;
                case 4:
                    pal_index >>= 4;
                    break;
                case 6:
                    pal_index >>= 6;
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
                PaintSurface(block_buff, block_pitch, buff_rect, color);

                mask_offset += 2;
                if (mask_offset >= 8) {
                    mask_offset = 0;
                }
            }
        }
    }
    if (P[0] > P[1] && P[2] > P[3]) {
        //1x2 pixel pattern
        offset = 12;
        for (int y = 0; y < 8; y+=2) {
            for (int x = 0; x < 8; x++) {
                uint8_t pal_index = B[byte_index] & (mask << mask_offset);

                switch (mask_offset)
                {
                case 2:
                    pal_index >>= 2;
                    break;
                case 4:
                    pal_index >>= 4;
                    break;
                case 6:
                    pal_index >>= 6;
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
                PaintSurface(block_buff, block_pitch, buff_rect, color);

                mask_offset += 2;
                if (mask_offset >= 8) {
                    mask_offset = 0;
                }
            }
        }
    }

    Rect buff_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };
    if (mark) {
        PaintSurface(block_buff, block_pitch, buff_rect, {255, 255, 255});
    }
    if (blit) {
        BlitSurface(block_buff, buff_rect, block_pitch, dst_buff, buff_rect, frame_pitch);
    }
    return offset;
}

//offset = 32 or 24;
int pattern_0x0A(uint8_t* data_stream, video* video, uint8_t* dst_buff, bool blit, bool mark)
{
    int offset = 0;

    uint8_t* block_buff = video_buffer.block_buffer;
    int block_pitch     = video_buffer.block_pitch;
    int frame_pitch     = video_buffer.pitch;
    palette* pal        = video_buffer.pal;

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

    uint8_t mask    = 0x03;
    int mask_offset = 0;
    int B_index     = 0;
    if (data_stream[0] <= data_stream[1]) {
        //quads
        offset = 32;
        for (int q = 0; q < 4; q++)
        {
            uint8_t* P = pattern.px32[q].P;         //q==0 =>TL, 1=>BL
            uint8_t* B = pattern.px32[q].B;         //q==2 =>TR, 3=>BR

            Rect start = {
                .x = 0,             // q==0 => x = 0
                .y = 0,             //         y = 0
                .w = 4,
                .h = 4,
            };
            switch (q)
            {
            case 1:                 // bottom left
                start.y = 4;        // q==1 => x = 0
                start.h = 8;        //         y = 4
                break;
            case 2:                 // top right
                start.x = 4;        // q==2 => x = 4
                start.w = 8;        //         y = 0
                break;
            case 3:                 // bottom right
                start.x = 4;        // q==3 => x = 4
                start.y = 4;        //         y = 4
                start.w = 8;
                start.h = 8;
                break;
            }

            for (int y = start.y; y < start.h; y++)
            {
                for (int x = start.x; x < start.w; x++)
                {
                    uint8_t P_index = B[B_index] & (mask << mask_offset);
                    switch (mask_offset)
                    {
                    case 2:
                        P_index >>= 2;
                        break;
                    case 4:
                        P_index >>= 4;
                        break;
                    case 6:
                        P_index >>= 6;
                        B_index++;
                        if (B_index >= 4) {
                            B_index = 0;
                        }
                    }
                    palette color = pal[P[P_index]];

                    block_buff[y*block_pitch + x*3 + 0] = color.r;
                    block_buff[y*block_pitch + x*3 + 1] = color.g;
                    block_buff[y*block_pitch + x*3 + 2] = color.b;

                    mask_offset += 2;
                    if (mask_offset >= 8) {
                        mask_offset =  0;
                    }
                }
            }
        }
    } else {
        //half & half
        offset = 24;
        for (int h = 0; h < 2; h++)
        {
            uint8_t* P = pattern.px24[h].P;
            uint8_t* B = pattern.px24[h].B;

            Rect start = {
                .x = 0,                 // h==0 => x = 0
                .y = 0,                 //         y = 0
                .w = 4,
                .h = 4,
            };
            if (pattern.px24[1].P[0] <= pattern.px24[1].P[1]) {
                //left and right half
                start.w = 4;
                start.h = 8;
                if (h == 1) {           // right half
                    start.x = 4;        // h==1 => x = 4
                    start.w = 8;        //         y = 0
                }
            } else {
                //top and bottom half
                start.w = 8;
                start.h = 4;
                if (h == 1) {           // bottom half
                    start.y = 4;        // h==1 => x = 0
                    start.h = 8;        //         y = 4
                }
            }

            for (int y = start.y; y < start.h; y++)
            {
                for (int x = start.x; x < start.w; x++)
                {
                    uint8_t P_index = B[B_index] & (mask << mask_offset);
                    switch (mask_offset)
                    {
                    case 2:
                        P_index >>= 2;
                        break;
                    case 4:
                        P_index >>= 4;
                        break;
                    case 6:
                        P_index >>= 6;
                        B_index++;
                        if (B_index >= 8) {
                            B_index = 0;
                        }
                    }
                    palette color = pal[P[P_index]];

                    block_buff[y*block_pitch + x*3 + 0] = color.r;
                    block_buff[y*block_pitch + x*3 + 1] = color.g;
                    block_buff[y*block_pitch + x*3 + 2] = color.b;

                    mask_offset += 2;
                    if (mask_offset >= 8) {
                        mask_offset  = 0;
                    }
                }
            }
        }
    }

    Rect buff_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };
    if (mark) {
        PaintSurface(block_buff, block_pitch, buff_rect, {255, 255, 255});
    }
    if (blit) {
        BlitSurface(block_buff, buff_rect, block_pitch, dst_buff, buff_rect, frame_pitch);
    }

    return offset;
}

//offset = 64;
int raw_pixels_0x0B(uint8_t* data_stream, video* video_buffer, uint8_t* dst_buff, bool blit, bool mark)
{
    uint8_t* block_buff = video_buffer->block_buffer;
    int buff_pitch      = video_buffer->block_pitch;
    int frame_pitch     = video_buffer->pitch;

    uint8_t P[64];
    palette* pal = video_buffer->pal;

    memcpy(P, data_stream, sizeof(P));

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            palette color = pal[P[y*8 + x]];
            block_buff[y*buff_pitch + x*3 + 0] = color.r;
            block_buff[y*buff_pitch + x*3 + 1] = color.g;
            block_buff[y*buff_pitch + x*3 + 2] = color.b;
        }
    }

    Rect buff_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };
    if (mark) {
        PaintSurface(block_buff, buff_pitch, buff_rect, {255, 255, 255});
    }
    if (blit) {
        BlitSurface(block_buff, buff_rect, buff_pitch, dst_buff, buff_rect, frame_pitch);
    }

    return 64;
}

//offset = 16;
int raw_pixels_0x0C(uint8_t* data_stream, video* video_buffer, uint8_t* dst_buff, bool blit, bool mark)
{
    uint8_t* block_buff = video_buffer->block_buffer;
    int buff_pitch      = video_buffer->block_pitch;
    int frame_pitch     = video_buffer->pitch;

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
            PaintSurface(block_buff, buff_pitch, dst_rect, color);
        }
    }

    Rect buff_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };
    if (mark) {
        PaintSurface(block_buff, buff_pitch, buff_rect, {255, 255, 255});
    }
    if (blit) {
        BlitSurface(block_buff, buff_rect, buff_pitch, dst_buff, buff_rect, frame_pitch);
    }

    return 16;
}

//offset = 4;
int raw_pixels_0x0D(uint8_t* data_stream, video*video_buffer, uint8_t* dst_buff, bool blit, bool mark)
{
    uint8_t* block_buff = video_buffer->block_buffer;
    int block_pitch     = video_buffer->block_pitch;
    int frame_pitch     = video_buffer->pitch;

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
            PaintSurface(block_buff, block_pitch, dst_rect, color);
        }
    }
    Rect buff_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };
    if (mark) {
        PaintSurface(block_buff, block_pitch, buff_rect, {255, 255, 255});
    }
    if (blit) {
        BlitSurface(block_buff, buff_rect, block_pitch, dst_buff, buff_rect, frame_pitch);
    }

    return 4;
}

//offset = 1;
//set the whole 8x8 pixels a solid color
int solid_frame_0x0E(uint8_t* data_stream, video* video_buffer, uint8_t* dst_buff, bool blit, bool mark)
{
    uint8_t* block_buff = video_buffer->block_buffer;
    int block_pitch     = video_buffer->block_pitch;
    int frame_pitch     = video_buffer->pitch;

    uint8_t pal_index = data_stream[0];
    Rect buff_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };

    palette color = video_buffer->pal[pal_index];

    PaintSurface(block_buff, block_pitch, buff_rect, color);
    if (mark) {
        PaintSurface(block_buff, block_pitch, buff_rect, {255, 255, 255});
    }
    if (blit) {
        BlitSurface(block_buff, buff_rect, block_pitch, dst_buff, buff_rect, frame_pitch);
    }

    return 1;
}

//offset = 2;
int dithered_0x0F(uint8_t* data_stream, video*video, uint8_t* dst_buff, bool blit, bool mark)
{
    uint8_t* block_buff = video_buffer.block_buffer;
    int block_pitch     = video_buffer.block_pitch;
    int frame_pitch     = video_buffer.pitch;

    uint8_t P[2] = {
        data_stream[0],
        data_stream[1]
    };

    palette* pal   = video_buffer.pal;
    palette color1 = pal[P[0]];
    palette color2 = pal[P[1]];

    bool hash = true;
    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            palette cur = hash ? color1 : color2;
            block_buff[y*block_pitch + x*3 +0] = cur.r;
            block_buff[y*block_pitch + x*3 +1] = cur.g;
            block_buff[y*block_pitch + x*3 +2] = cur.b;
            hash = !hash;
        }
        hash = !hash;
    }

    Rect block_rect = {
        .x = 0,
        .y = 0,
        .w = 8,
        .h = 8,
    };
    if (mark) {
        PaintSurface(block_buff, block_pitch, block_rect, {255, 255, 255});
    }
    if (blit) {
        BlitSurface(block_buff, block_rect, block_pitch, dst_buff, block_rect, frame_pitch);
    }

    return 2;
}

//frame_buffer points to the current pixel block passed in from parse_video_data()
//data_stream  points to the current position for the pixel information
//x_ & y_offset are the current offset into the frame_buffer (already applied to frame_buffer)
int parse_video_encode(uint8_t op, uint8_t* data_stream, uint8_t* frame_buffer, int x_offset, int y_offset)
{
    video_buffer.encode_type[op]++;

    bool* allow_blit = video_buffer.allow_blit;
    bool* blit_mark  = video_buffer.blit_marker;
    int* data_offset = video_buffer.data_offset;

#ifdef DEBUG
    if (y_offset >= 16 && y_offset < 32) {
        if (x_offset >= 8 && x_offset < 16) {
            printf("this opdcode %0x\n", op);
        }
    }
#endif

    int offset = 0;
    switch (op)
    {
    case 0x00:
        offset = blockCopy_0x00(&data_stream[data_offset[op]], frame_buffer, x_offset, y_offset, allow_blit[op], blit_mark[op]);
        //offset = 0;
        break;
    case 0x01:
        offset = 0;
        break;
    case 0x02:
        offset = cornerCopy_0x02(&data_stream[data_offset[op]], x_offset, y_offset, frame_buffer, allow_blit[op], blit_mark[op]);
        //offset = 1;
        break;
    case 0x03:
        offset = cornerCopy_0x03(&data_stream[data_offset[op]],x_offset, y_offset, frame_buffer, allow_blit[op], blit_mark[op]);
        //offset = 1;
        break;
    case 0x04:
        offset = symmetricCopy_0x04(&data_stream[data_offset[op]],x_offset, y_offset, frame_buffer, allow_blit[op], blit_mark[op]);
        //offset = 1;
        break;
    case 0x05:
        offset = symmetricCopy_0x05(&data_stream[data_offset[op]],x_offset, y_offset, frame_buffer, allow_blit[op], blit_mark[op]);
        //offset = 2;
        break;
    case 0x06:
        offset = 0;
        //no opcode 0x06 found so far
        break;
    case 0x07:
        offset = pattern_0x07(&data_stream[data_offset[op]], &video_buffer, frame_buffer, allow_blit[op], blit_mark[op]);
        //offset = 10;
        //  or
        //offset = 4;
        break;
    case 0x08:
        offset = pattern_0x08(&data_stream[data_offset[op]], frame_buffer, allow_blit[op], blit_mark[op]);
        //offset = 16;
        //  or
        //offset = 12;
        break;
    case 0x09:
        offset = pattern_0x09(&data_stream[data_offset[op]], &video_buffer, frame_buffer, allow_blit[op], blit_mark[op]);
        //offset = 20;
        //offset = 8;
        //offset = 12;
        //offset = 12;
        break;
    case 0x0A:
        offset = pattern_0x0A(&data_stream[data_offset[op]], &video_buffer, frame_buffer, allow_blit[op], blit_mark[op]);
        //offset = 32;
        //offset = 24;
        break;
    case 0x0B:
        offset = raw_pixels_0x0B(&data_stream[data_offset[op]], &video_buffer, frame_buffer, allow_blit[op], blit_mark[op]);
        //offset = 64;
        break;
    case 0x0C:
        offset = raw_pixels_0x0C(&data_stream[data_offset[op]], &video_buffer, frame_buffer, allow_blit[op], blit_mark[op]);
        //offset = 16;
        break;
    case 0x0D:
        offset = raw_pixels_0x0D(&data_stream[data_offset[op]], &video_buffer, frame_buffer, allow_blit[op], blit_mark[op]);
        //offset = 4;
        break;
    case 0x0E:
        offset = solid_frame_0x0E(&data_stream[data_offset[op]], &video_buffer, frame_buffer, allow_blit[op], blit_mark[op]);
        //offset = 1;
        break;
    case 0x0F:
        offset = dithered_0x0F(&data_stream[data_offset[op]], &video_buffer, frame_buffer, allow_blit[op], blit_mark[op]);
        //offset = 2;
        break;

    default:
        break;
    }

    return offset;
}

void parse_video_data(uint8_t* buffer)
{
    uint8_t* data_stream = buffer;
    uint8_t* map_stream  = video_buffer.map_stream;

    int encode_type_previous_frame[0xf];
    int encode_type_per_frame[0xf];
#ifdef DEBUG
    for (int i = 0; i <= 0xF; i++) {
        encode_type_previous_frame[i] = video_buffer.encode_type[i];
    }
#endif

    //apparently the video buffers are only swapped based on a single bit flag
    //  set in the first 14 bytes (might be 16 in later encodes) of the actual
    //  frame(?) data, not when you go to blit the buffer to the render engine
    // swap buffers
    #define SWAP_BUFFER_BIT         (0x01)
    uint16_t flags = *(uint16_t*)&buffer[12];
    if (flags & SWAP_BUFFER_BIT) {
        if (video_buffer.pxls == video_buffer.frnt_buffer) {
            video_buffer.pxls  = video_buffer.back_buffer;
        } else {
            video_buffer.pxls  = video_buffer.frnt_buffer;
        }
    }
    //and apparently when you swap the buffers based on the bit flag
    //  the next frame the be drawn to is whatever video_buffer.pxls
    //  points to (as assigned here)
    uint8_t* next_frame = video_buffer.pxls;

    // x_offset & y_offset == offset into the framebuffer in pixels
    int x_offset    = 0;
    int y_offset    = 0;
    uint8_t mask    = video_buffer.encode_bits;         //debug global to see which bits go first
    int data_offset = video_buffer.data_offset_init;    //debug global to find correct offset
    int frame_pitch = video_buffer.pitch;
    for (int i = 0; i < video_buffer.map_size*2; i++)
    {
        uint8_t enc = map_stream[i/2] & mask;
        if (mask == 0xF0) {
            enc >>= 4;
        }
        mask = ~mask;

        data_offset += parse_video_encode(
                        enc,
                        &data_stream[data_offset],
                        &next_frame[y_offset*frame_pitch + x_offset*3],
                        x_offset,
                        y_offset
        );

        x_offset += 8;
        if (x_offset*3 >= frame_pitch) {
            x_offset  = 0;
            y_offset += 8;
        }
    }
#ifdef DEBUG
    for (int i = 0; i <= 0xf; i++) {
        encode_type_per_frame[i] = video_buffer.encode_type[i] - encode_type_previous_frame[i];
    }
#endif
}