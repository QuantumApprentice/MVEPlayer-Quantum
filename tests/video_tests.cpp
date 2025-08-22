#include <memory.h>
#include <stdlib.h>
#include "parse_video.h"
#include "parse_opcodes.h"

video video_buffer;

void test_blockCopy_0x00(uint8_t* out_buffer);

#define PXL_DEPTH           (3)
#define TEST_BUFF_SIZE      (64*PXL_DEPTH)

int main()
{
    uint8_t out_buffer[TEST_BUFF_SIZE] = {};

    video_buffer.frnt_buffer = (uint8_t*)malloc(sizeof(out_buffer));
    video_buffer.back_buffer = (uint8_t*)malloc(sizeof(out_buffer));

    video_buffer.pitch = video_buffer.block_pitch;
    video_buffer.pxls  = video_buffer.frnt_buffer;

    test_blockCopy_0x00(out_buffer);

    free(video_buffer.frnt_buffer);
    free(video_buffer.back_buffer);

    printf("wtf? why are you segfault-ing?\n");
    printf("end of line\n-------------------\n");

    return 0;
}

void print_buff(uint8_t* buffer, int w, int h, int pitch, int depth)
{
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            printf("%d ", buffer[y*pitch + x*depth]);
        }
        printf("\n");
    }
}

//blockCopy_0x00() copies from whichever the back-buffer is
//if pxls is pointing at frnt_buffer, then it copies from back_buffer, and vice-versa
//data_offset should be 0 for this?
void test_blockCopy_0x00(uint8_t* out_buffer)
{
    memset(video_buffer.frnt_buffer, 0xF0, TEST_BUFF_SIZE);
    memset(video_buffer.back_buffer, 0x0F, TEST_BUFF_SIZE);

    //test1 - copy from same sized buffer, 0 offset
    int compare = memcmp(video_buffer.pxls, out_buffer, TEST_BUFF_SIZE);
    printf("before blockCopy_0x00() compare: %d\n", compare);

    int data_offset = blockCopy_0x00(NULL, out_buffer, 0, 0, true, false);

    compare = memcmp(video_buffer.pxls, out_buffer, TEST_BUFF_SIZE);
    printf("after blockCopy_0x00 compare: %d\n", compare);
    printf("data_offset: %d (should be 0?)\n", data_offset);


    //test2 - copy from different sized buffer, x & y offset


    int block_pitch    = video_buffer.block_pitch;
    video_buffer.pitch = block_pitch * 2;

    static uint8_t test_buff[TEST_BUFF_SIZE*4] = {
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,     0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,     0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,     0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,     0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,     0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,     0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,     0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,     0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,

        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,     0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,     0,0,0, 0,0,0, 0,0,0, 9,9,9, 9,9,9, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,     0,0,0, 0,0,0, 9,9,9, 9,9,9, 9,9,9, 9,9,9, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,     0,0,0, 9,9,9, 9,9,9, 9,9,9, 9,9,9, 9,9,9, 9,9,9, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,     0,0,0, 0,0,0, 9,9,9, 9,9,9, 9,9,9, 9,9,9, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,     0,0,0, 0,0,0, 0,0,0, 9,9,9, 9,9,9, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,     0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,     0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
    };

    video_buffer.back_buffer = test_buff;

    printf("before memset\n");
    print_buff(out_buffer, 8, 8, block_pitch, PXL_DEPTH);


    memset(out_buffer, 0, TEST_BUFF_SIZE);

    printf("before copy\n");
    print_buff(out_buffer, 8, 8, block_pitch, PXL_DEPTH);

    data_offset = blockCopy_0x00(NULL, out_buffer, 8,8, true, false);

    uint8_t check_buff[TEST_BUFF_SIZE] = {
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 9,9,9, 9,9,9, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 9,9,9, 9,9,9, 9,9,9, 9,9,9, 0,0,0, 0,0,0,
        0,0,0, 9,9,9, 9,9,9, 9,9,9, 9,9,9, 9,9,9, 9,9,9, 0,0,0,
        0,0,0, 0,0,0, 9,9,9, 9,9,9, 9,9,9, 9,9,9, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 9,9,9, 9,9,9, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
        0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,
    };

    compare = memcmp(check_buff, out_buffer, TEST_BUFF_SIZE);
    printf("after blockCopy_0x00() compare: %d\n", compare);
    printf("data_offset: %d (should be 0?)\n", data_offset);
}