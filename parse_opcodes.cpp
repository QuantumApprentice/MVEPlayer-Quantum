#include <stdint.h>
#include "parse_opcodes.h"
#include "parse_video.h"
#include "parse_audio.h"


void parse_opcode(opcodeinfo op, uint8_t* buffer)
{
    // printf("")
    switch (op.type)
    {
    case 0x00:
        break;
    case 0x01:
        //end of chunk
        //do nothing?
        break;
    case 0x02:
        create_timer(buffer);
        break;
    case 0x03:
        break;
    case 0x04:
        printf("0x04: start/stop audio (skipping for now)\n");
        break;
    case 0x05:
        init_video_buffer(buffer, op.version);
        break;
    case 0x06:
        break;
    case 0x07:
        send_buffer_to_display(buffer);
        break;
    case 0x08:  //fall through to 0x09 (both are audio)
    case 0x09:
        parse_audio_frame(buffer);
        break;
    case 0x0A:  //Initialize Video Mode
        init_video_mode(buffer);
        break;
    case 0x0B:
        break;
    case 0x0C:
        init_palette(buffer);
        break;
    case 0x0D:
        break;
    case 0x0E:
        break;
    case 0x0F:
        parse_decoding_map(buffer, op.size);
        break;
    case 0x10:
        break;
    case 0x11:
        parse_video_data(buffer);
        break;
    case 0x12:
        break;
    case 0x13:
        break;
    case 0x14:
        break;
    case 0x15:
        //unknown
        //do nothing?
        break;

    default:
        printf("missing or wrong opcode: %d\n", op.type);
        break;
    }
}
