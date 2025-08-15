#pragma once
#include <stdint.h>
#include "parse_video.h"

typedef struct Rect {
    int x;
    int y;
    int w;
    int h;
} Rect;

void BlitSurface(uint8_t* src, Rect src_rect, uint8_t* dst, Rect dst_rect, int src_pitch, int dst_pitch);
void PaintSurface(uint8_t* dst, int pitch, Rect brush_rect, palette color);
void ClearSurface(video* dst);
void FreeSurface(video* src);

// void print_SURFACE_pxls(video* src);