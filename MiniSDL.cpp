#include <stdlib.h>
#include <assert.h>
#include <memory.h>

#include "MiniSDL.h"

//src, src_rect, dst, dst_rect
//both surfaces must be same pixel width (RGB/RGBA etc)
//src_rect.w & h must be == dst_rect
void BlitSurface(uint8_t* src, Rect src_rect, uint8_t* dst, Rect dst_rect, int src_pitch, int dst_pitch)
{
    assert(src_rect.w == dst_rect.w);
    assert(src_rect.h == dst_rect.h);
    //set starting position for top left corner of rectangle to copy
    uint8_t* src_pxls = &src[src_rect.y*src_pitch + src_rect.x*3];
    uint8_t* dst_pxls = &dst[dst_rect.y*dst_pitch + dst_rect.x*3];

    //copy each row of src rectangle to dst surface
    for (int row = 0; row < src_rect.h; row++) {
        memcpy(dst_pxls, src_pxls, src_pitch);
        src_pxls += src_pitch;
        dst_pxls += dst_pitch;
    }
}

//sets dst->pxls to 'color' palette index
//brush_rect determines section to color
void PaintSurface(uint8_t* dst, int pitch, Rect brush_rect, palette color)
{
    if (dst == nullptr) {
        return;
    }
    // printf("color: %08x\n", color);
    uint8_t* dst_pxls = &dst[brush_rect.y*pitch + brush_rect.x*3];
    //copy each row of src rectangle to dst surface
    for (int row = 0; row < brush_rect.h; row++) {
        for (int col = 0; col < brush_rect.w; col++) {
            dst_pxls[0+col*3] = color.r;
            dst_pxls[1+col*3] = color.g;
            dst_pxls[2+col*3] = color.b;
            // dst_pxls[3+col*3] = color.a;
        }
        dst_pxls += pitch;
    }
}

void ClearSurface(video* dst)
{
    if (dst == nullptr) {
        return;
    }
    uint8_t* pxls = dst->video_buffer;
    memset(pxls, 0, dst->render_w*dst->render_h);
}

void FreeSurface(video* src)
{
    uint8_t* pxls_ptr = ((uint8_t*)src) + sizeof(video);
    if (src->video_buffer != pxls_ptr) {
        free(src->video_buffer);
    }
    free(src);
}

// void print_SURFACE_pxls(video* src)
// {
//     for (int y = 0; y < src->h; y++)
//     {
//         for (int x = 0; x < src->pitch; x+=src->channels)
//         {
//             if (src->channels == 4) {
//                 Color pxl;
//                 memcpy(&pxl, &src->pxls[y*src->pitch + x], sizeof(Color));
//                 printf("%02x%02x%02x%02x", pxl.r, pxl.g, pxl.b, pxl.a);
//             } else
//             if (src->channels == 3) {
//                 Color pxl;
//                 memcpy(&pxl, &src->pxls[y*src->pitch + x], 3);
//                 printf("%02x%02x%02x", pxl.r, pxl.g, pxl.b);
//             } else
//             if (src->channels == 1) {
//                 printf("%02x", src->pxls[y*src->pitch + x]);
//             }
//         }
//         printf("\n");
//     }
// }