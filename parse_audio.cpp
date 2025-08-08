#include "parse_audio.h"
#include <memory.h>

struct audio_frame {
    uint16_t index;
    uint16_t mask;
    uint16_t length;
};

void init_audio(FILE* fileptr)
{
    printf("skipping audio for now\n");
    return;
}

void parse_audio_frame(uint8_t* buffer)
{
    audio_frame frame;
    memcpy(&frame, buffer, sizeof(frame));

    printf("audio frame - index: %d, mask: %d, length: %d\n", frame.index, frame.mask, frame.length);

}