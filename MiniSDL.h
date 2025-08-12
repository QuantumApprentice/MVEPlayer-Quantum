#pragma once
#include <stdint.h>
#include "parse_video.h"

typedef struct Rect {
    int x;
    int y;
    int w;
    int h;
} Rect;

void BlitSurface( video* src, Rect src_rect, video* dst, Rect dst_rect);
void PaintSurface(uint8_t* dst, int pitch, Rect brush_rect, palette color);
void ClearSurface(video* dst);
void FreeSurface(video* src);

// void print_SURFACE_pxls(video* src);