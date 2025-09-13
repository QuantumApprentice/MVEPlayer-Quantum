#include <stdint.h>
#include "parse_opcodes.h"
#include "parse_video.h"
#include "parse_audio.h"


void parse_opcode(opcodeinfo op, uint8_t* buffer)
{
    switch (op.type)
    {
    case 0x00:
        printf("skipping opcode 0x00: End of stream\n");
        break;
    case 0x01:
        printf("skipping opcode 0x01: End of chunk (what is this supposed to do?)\n");
        //end of chunk
        //do nothing?
        break;
    case 0x02:
        create_timer(buffer);
        break;
    case 0x03:
        printf("skipping opcode 0x03: Init audio buffers\n");
        break;
    case 0x04:
        printf("skipping opcode 0x04: start/stop audio (skipping for now)\n");
        break;
    case 0x05:
        init_video_buffer(buffer, op.version);
        break;
    case 0x06:
        printf("skipping opcode 0x06: unkown\n");
        break;
    case 0x07:
        send_buffer_to_display(buffer);
        break;
    case 0x08:  //data: fall through to 0x09 (both are audio)
        printf("parsing  opcode 0x08: audio data\n");
    case 0x09:  //silence (but processes data instead?)
        printf("parsing  opcode 0x09: audio silence\n");
        parse_audio_frame(buffer);
        break;
    case 0x0A:  //Initialize Video Mode
        init_video_mode(buffer);
        break;
    case 0x0B:
        printf("skipping opcode 0x0B: Create gradient\n");
        break;
    case 0x0C:
        init_palette(buffer);
        break;
    case 0x0D:
        printf("skipping opcode 0x0D: Set palette compressed\n");
        break;
    case 0x0E:
        printf("skipping opcode 0x0E: Unknown\n");
        break;
    case 0x0F:
        parse_decoding_map(buffer, op.size);
        break;
    case 0x10:
        printf("skipping opcode 0x10: Unknown (uses 3 data streams?)\n");
        break;
    case 0x11:
        printf("parsing  opcode 0x11: parse video data\n");
        parse_video_data(buffer);
        break;
    case 0x12:
        printf("skipping opcode 0x12: Unused\n");
        break;
    case 0x13:
        printf("skipping opcode 0x13: Unknown (always 0x84 bytes?)\n");
        break;
    case 0x14:
        printf("skipping opcode 0x14: Unused\n");
        break;
    case 0x15:
        printf("skipping opcode 0x15: Unknown\n");
        //unknown
        //do nothing?
        break;

    default:
        printf("missing or wrong opcode: %d\n", op.type);
        break;
    }
}
