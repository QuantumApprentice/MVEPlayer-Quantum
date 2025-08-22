#include <stdio.h>
#include "parse_opcodes.h"
#include "parse_video.h"
#include "parse_video.cpp"

int main()
{
    uint8_t out_buffer[64*3] = {};

    video_buffer.frnt_buffer = (uint8_t*)malloc(sizeof(out_buffer));
    video_buffer.back_buffer = (uint8_t*)malloc(sizeof(out_buffer));
    memset(video_buffer.frnt_buffer, 0xF0, sizeof(out_buffer));
    memset(video_buffer.back_buffer, 0xF0, sizeof(out_buffer));
    video_buffer.pitch = video_buffer.block_pitch;
    video_buffer.pxls  = video_buffer.frnt_buffer;



    int data_offset = blockCopy_0x00(NULL, out_buffer, 0, 0, true, false);

    int compare = memcmp(video_buffer.pxls, out_buffer, sizeof(out_buffer));

    printf("Compare: %d\n", compare);


}