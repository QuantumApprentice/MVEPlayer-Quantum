#include "io_Platform.h"
#include "parse_video.h"
extern video video_buffer;

#include <stdlib.h>
#include <string.h>



void file_drop_callback(GLFWwindow *window, int count, const char** files)
{
    size_t size = 0;
    for (int i = 0; i < count; i++) {
        size += strlen(files[i]) + 1;
    }
    video_buffer.filename = (char*)malloc(size);

    int len = strlen(files[0]) + 1;
    memcpy(video_buffer.filename, files[0], len);

    video_buffer.file_drop_frame = true;
}