#pragma once
#include <stdint.h>

struct subtitle
{
    char* subtitle  = nullptr;
    int length      = 0;
    int frame_start = 0;
};

struct subtitles
{
    uint8_t* data      = nullptr;
    int data_size      = 0;

    subtitle* sub_list = nullptr;
    int count          = 0;
};

subtitles load_subs(char* path);
