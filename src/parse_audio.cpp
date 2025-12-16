#include "parse_audio.h"

#include "parse_audio_alsa.h"
// #include "parse_audio_pipewire.h"
#include "parse_audio_pipewire_thread.h"
#include "ring_buffer.h"
#include "parse_audio_sdl.h"

extern video video_buffer;
struct audio_handle audio;

// BakerStaunch
//  You would probably use an approach like this
//  to make it more generic across different audio APIs
//  - have your code write into your own ring buffer
//  and then when you're playing audio
//  - copy from your ring buffer into the destination one
//  (e.g. with a write call in alsa or a ring buffer copy for pipewire)
//  Then the step after that is move the code that interfaces
//  with the audio API to its own dedicated thread
//  so you can reduce latency and have a small buffer size overall
//  (and handle underruns by passing silence to the audio api)

int delta_table[256] = {
        0,      1,      2,      3,      4,      5,      6,      7,      8,      9,     10,     11,     12,     13,     14,     15,
       16,     17,     18,     19,     20,     21,     22,     23,     24,     25,     26,     27,     28,     29,     30,     31,
       32,     33,     34,     35,     36,     37,     38,     39,     40,     41,     42,     43,     47,     51,     56,     61,
       66,     72,     79,     86,     94,    102,    112,    122,    133,    145,    158,    173,    189,    206,    225,    245,
      267,    292,    318,    348,    379,    414,    452,    493,    538,    587,    640,    699,    763,    832,    908,    991,
     1081,   1180,   1288,   1405,   1534,   1673,   1826,   1993,   2175,   2373,   2590,   2826,   3084,   3365,   3672,   4008,
     4373,   4772,   5208,   5683,   6202,   6767,   7385,   8059,   8794,   9597,  10472,  11428,  12471,  13609,  14851,  16206,
    17685,  19298,  21060,  22981,  25078,  27367,  29864,  32589, -29973, -26728, -23186, -19322, -15105, -10503,  -5481,     -1,
        1,      1,   5481,  10503,  15105,  19322,  23186,  26728,  29973, -32589, -29864, -27367, -25078, -22981, -21060, -19298,
   -17685, -16206, -14851, -13609, -12471, -11428, -10472,  -9597,  -8794,  -8059,  -7385,  -6767,  -6202,  -5683,  -5208,  -4772,
    -4373,  -4008,  -3672,  -3365,  -3084,  -2826,  -2590,  -2373,  -2175,  -1993,  -1826,  -1673,  -1534,  -1405,  -1288,  -1180,
    -1081,   -991,   -908,   -832,   -763,   -699,   -640,   -587,   -538,   -493,   -452,   -414,   -379,   -348,   -318,   -292,
     -267,   -245,   -225,   -206,   -189,   -173,   -158,   -145,   -133,   -122,   -112,   -102,    -94,    -86,    -79,    -72,
      -66,    -61,    -56,    -51,    -47,    -43,    -42,    -41,    -40,    -39,    -38,    -37,    -36,    -35,    -34,    -33,
      -32,    -31,    -30,    -29,    -28,    -27,    -26,    -25,    -24,    -23,    -22,    -21,    -20,    -19,    -18,    -17,
      -16,    -15,    -14,    -13,    -12,    -11,    -10,     -9,     -8,     -7,     -6,     -5,     -4,     -3,     -2,     -1
};


void init_audio(uint8_t* buff, uint8_t version)
{
    #pragma pack(push,1)
    struct audio_info_v0 {
        int16_t unk;
        int16_t flags;
        int16_t sample_rate;
        int16_t min_buff_len;
    } v0;
    struct audio_info_v1 {
        int16_t unk;
        int16_t flags;
        int16_t sample_rate;
        int32_t min_buff_len;
    } v1;
    #pragma pack(pop)

    int16_t unk;
    int16_t flags;
    int16_t sample_rate;
    int32_t min_buff_len;


    if (version == 0) {
        memcpy(&v0, buff, sizeof(v0));
        unk          = v0.unk;
        flags        = v0.flags;
        sample_rate  = v0.sample_rate;
        min_buff_len = v0.min_buff_len;
    }
    if (version == 1) {
        memcpy(&v1, buff, sizeof(v1));
        unk          = v1.unk;
        flags        = v1.flags;
        sample_rate  = v1.sample_rate;
        min_buff_len = v1.min_buff_len;
    }

    int channels = 1;   //TODO: might need to experiment when converting mono to stereo
    int bits     = 8;
    int compress = false;
    if (flags & 0x1) {   //0=mono, 1=stereo
        channels = 2;
    }
    if (flags & 0x2) {   //0=8-bit, 1=16-bit
        bits     = 16;
    }
    if (flags & 0x4) {   //using compression
        compress = true;
    }

    audio.audio_channels            = channels;
    audio.audio_rate                = sample_rate;
    audio.audio_bits                = bits;
    audio.audio_compress            = compress;
    audio.decode_min_buff_len       = min_buff_len;


    //get size to allocate
    // int fps = 1000000.0f/(video_buffer.timer.rate*video_buffer.timer.subdivision);
    int fps         = 15;
    int samples_per_frame = audio.audio_rate/fps;


    // video_buffer.audio_samples_per_frame = samples_per_frame;
    audio.audio_samples_per_frame = samples_per_frame;


    int buff_size;
    if (audio.decode_min_buff_len > 0) {
        buff_size = audio.decode_min_buff_len;
    } else {
        buff_size = samples_per_frame;
    }
    //allocate buffer
    if (!audio.decode_buff) {
        audio.decode_buff = (int16_t*)calloc(buff_size*sizeof(int16_t), 1);
        audio.decode_buff_size = buff_size;
    }

    video_buffer.audio = &audio;


    switch (video_buffer.audio_pipe)
    {
    case ALSA:
        init_audio_alsa(&audio);
        break;
    case PIPEWIRE:
        // init_audio_pipewire(&audio);
        break;
    case PIPEWIRETHREAD:
        init_audio_pipewire(&audio);
        break;
    case RINGBUFFER:
        init_audio_pipewire(&audio);
        init_ring(buff_size * audio.audio_channels);
        break;
    case SDL_:
        init_sdl(&audio);
        init_ring(buff_size * audio.audio_channels);
        break;
    default:
        break;
    }
}


//stereo
void decompress_8(uint8_t* buff, int len)
{
    int8_t* audio_buff = (int8_t*)audio.decode_buff;
    if (audio.audio_channels == 1) {     //mono
        static int8_t last = buff[0];
        for (int i = 0; i < len; i++)
        {
            int8_t curr = delta_table[buff[i]] + last;
            audio_buff[i] = curr*video_buffer.audio_volume /8;
            last = curr;
        }
    }

    if (audio.audio_channels == 2) {     //stereo
        static int8_t l_last = buff[0];
        static int8_t r_last = buff[1];
        for (int i = 2; i < len; i++)
        {
            int8_t l_curr = delta_table[buff[i*2 +0]] + l_last;
            int8_t r_curr = delta_table[buff[i*2 +1]] + r_last;

            audio_buff[i*2 +0] = (l_curr)*video_buffer.audio_volume /8;
            audio_buff[i*2 +1] = (r_curr)*video_buffer.audio_volume /8;

            l_last = l_curr;
            r_last = r_curr;
        }
    }

}


//stereo
void decompress_16(uint8_t* buff, int len)
{
    int16_t* buff_16 = (int16_t*)buff;
    int16_t* audio_buff = audio.decode_buff;
    if (audio.audio_channels == 1) {     //mono
        // int16_t last = buff_16[0];
        // for (int i = 0; i < len; i++)
        // {
        //     int16_t curr = delta_table_[buff[i]] + last;
        //     audio_buff[i] = curr*video_buffer.audio_volume/8;
        //     last = curr;
        // }

        int16_t l_last = buff_16[0];
        int16_t r_last = buff_16[0];
        for (int i = 0; i < len; i++)
        {
            int idx   = i*2;
            int val_l = buff[i];
            // int val_r = buff[idx+1];
            int16_t l_curr = delta_table[val_l] + l_last;
            int16_t r_curr = delta_table[val_l] + r_last;

            audio_buff[idx +0] = l_curr *video_buffer.audio_volume /8;
            audio_buff[idx +1] = r_curr *video_buffer.audio_volume /8;

            l_last = l_curr;
            r_last = r_curr;
        }
    }
    if (audio.audio_channels == 2) {     //stereo
        int16_t l_last = buff_16[0];
        int16_t r_last = buff_16[1];

        for (int i = 2; i < len/2; i++)
        {
            int idx   = i*2;
            int val_l = buff[idx+0];
            int val_r = buff[idx+1];
            int16_t l_curr = delta_table[val_l] + l_last;
            int16_t r_curr = delta_table[val_r] + r_last;

            audio_buff[idx +0] = l_curr *video_buffer.audio_volume /8;
            audio_buff[idx +1] = r_curr *video_buffer.audio_volume /8;

            l_last = l_curr;
            r_last = r_curr;
        }
    }

}

#pragma pack(push,1)
struct audio_frame {
    uint16_t index;
    uint16_t mask;
    uint16_t length;
    uint8_t data[];
};
#pragma pack(pop)

void parse_audio_frame(uint8_t* buff, opcodeinfo op)
{

    audio_frame* frame = (audio_frame*)buff;
    //TODO: docs are wrong for frame.length
    //      actual input length is frame.length/(bytes per channel)
    //      actual output length is frame.length

    static int frame_count = 0;
    frame_count++;
    printf("audio frame count: %d length: %d\n",frame_count, frame->length);

    // audio.audio_calc_rate += frame->length;
    if (audio.audio_compress == 0) {
        // memcpy(audio.audio_buff, frame->data, op.size-8);
        int8_t* audio_buff_8 = (int8_t*)audio.decode_buff;
        if (audio.audio_bits == 8) {
            if (audio.audio_channels == 1) {
                for (int i = 0; i < op.size-8; i++)
                {
                    int8_t sample = frame->data[i];
                    audio_buff_8[i] = sample*video_buffer.audio_volume/8;
                }
            }
            if (audio.audio_channels == 2) {
                for (int i = 0; i < (op.size-8)/2; i++)
                {
                    int8_t l_curr = frame->data[i*2 +0];
                    int8_t r_curr = frame->data[i*2 +1];
                    audio_buff_8[i*2 +0] = l_curr*video_buffer.audio_volume/8;
                    audio_buff_8[i*2 +1] = r_curr*video_buffer.audio_volume/8;
                }
            }
        }
        // convert from 8 bit mono to 16 bit stereo?
        // for (int i = 0; i < op.size-8; i++)
        // {
        //     int16_t sample = *(int16_t*)frame->data[i];
        //     audio.audio_buff[i] = sample*video_buffer.audio_volume/8;
        // }
    }

    if (audio.audio_compress == 1) {
        uint8_t decompress_buff[65536] = {0};
        memcpy(decompress_buff, frame->data, op.size-8);
        if (audio.audio_bits == 8 ) {
            decompress_8(decompress_buff, op.size-8);
        }
        if (audio.audio_bits == 16) {
            decompress_16(decompress_buff, op.size-8);
        }

    }


    switch (video_buffer.audio_pipe)
    {
    case ALSA:
        parse_audio_frame_alsa(&audio, frame->length);
        break;
    case PIPEWIRE:
        // parse_audio_frame_pipewire();
        break;
    case PIPEWIRETHREAD:
        push_samples((uint8_t*)audio.decode_buff, frame->length);
        break;
    case RINGBUFFER:
        copy_to_ring((uint8_t*)audio.decode_buff, frame->length);
        break;
    case SDL_:
        copy_to_ring((uint8_t*)audio.decode_buff, frame->length);
        play_sdl(video_buffer.pause);
        break;
    default:
        break;
    }
}

void pause_audio(bool pause)
{
    switch (video_buffer.audio_pipe)
    {
    case ALSA:
        break;
    case PIPEWIRE:
        // parse_audio_frame_pipewire();
        break;
    case PIPEWIRETHREAD:
        break;
    case RINGBUFFER:
        break;
    case SDL_:
        play_sdl(video_buffer.pause);
        break;
    default:
        break;
    }
}

void shutdown_audio()
{
    switch (video_buffer.audio_pipe)
    {
    case ALSA:
        shutdown_audio_alsa;
        break;
    case PIPEWIRE:
        // shutdown_audio_pipewire();
        break;
    case PIPEWIRETHREAD:
        shutdown_audio_pipewire();
        break;
    case RINGBUFFER:
        free_ring();
        shutdown_audio_pipewire();
        break;
    case SDL_:
        free_ring();
        close_sdl();
        break;
    default:
        break;
    }

    free(audio.decode_buff);
    audio.decode_buff = NULL;
}