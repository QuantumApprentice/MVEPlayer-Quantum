#include "parse_audio.h"
#include <memory.h>
#include <math.h>


#include <alsa/asoundlib.h>

#include "io_Timer.h"
#include "parse_opcodes.h"

extern video video_buffer;

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

#pragma pack(push,1)
struct audio_frame {
    uint16_t index;
    uint16_t mask;
    uint16_t length;
    uint8_t data[];
};
#pragma pack(pop)


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


    int channels = 1;
    int bits     = 8;
    int compress = false;
    if (flags & 0x1) {   //0=mono, 1=stereo
        channels = 2;
    }
    if (flags & 0x2) {   //0=8-bit, 1=16-bit
        bits = 16;
    }
    if (flags & 0x4) {   //using compression
        compress = true;
    }

    video_buffer.audio_channels = channels;
    video_buffer.audio_rate     = sample_rate;
    video_buffer.audio_bits     = bits;
    video_buffer.audio_compress = compress;



    uint32_t pcm, tmp, dir;
    // snd_pcm_t* pcm_handle;
    // snd_pcm_hw_params_t* params;
    // snd_pcm_uframes_t frames;

    pcm = snd_pcm_open(&video_buffer.pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't open '%s' PCM devicel. %s\n", "default", snd_strerror(pcm));
    }

    snd_pcm_hw_params_alloca(&video_buffer.audio_params);
    snd_pcm_hw_params_any(video_buffer.pcm_handle, video_buffer.audio_params);

    //set params
    pcm = snd_pcm_hw_params_set_access(video_buffer.pcm_handle, video_buffer.audio_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm));
    }

    pcm = snd_pcm_hw_params_set_format(video_buffer.pcm_handle, video_buffer.audio_params, SND_PCM_FORMAT_S16_LE);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set format. %s\n",snd_strerror(pcm));
    }

    pcm = snd_pcm_hw_params_set_channels(video_buffer.pcm_handle, video_buffer.audio_params, video_buffer.audio_channels);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set channels count. %s\n", snd_strerror(pcm));
    }

    pcm = snd_pcm_hw_params_set_rate_near(video_buffer.pcm_handle, video_buffer.audio_params, &video_buffer.audio_rate, 0);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set rate. %s\n", snd_strerror(pcm));
    }

    //write params
    pcm = snd_pcm_hw_params(video_buffer.pcm_handle, video_buffer.audio_params);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set hardware params. %s\n", snd_strerror(pcm));
    }

    //resume info?
    printf("PCM name: %s\n", snd_pcm_name(video_buffer.pcm_handle));
    printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(video_buffer.pcm_handle)));

    //so we _set_ the params earlier
    //  and here we _get_ the params
    //  to make sure they were set?
    snd_pcm_hw_params_get_channels(video_buffer.audio_params, &tmp);
    printf("channels: %i ", tmp);

    if (tmp == 1) {
        printf("(mono)\n");
    } else
    if (tmp == 2) {
        printf("(stereo)\n");
    }

    snd_pcm_hw_params_get_rate(video_buffer.audio_params, &tmp, 0);
    printf("rate: %d bps file-rate: %d\n", tmp, video_buffer.audio_rate);
    // printf("seconds: %d (although really what use is this?)\n", video_buffer.seconds);

    // allocate? buffer to hold single period?
    //wait, does this assign something to frames?
    snd_pcm_hw_params_get_period_size(video_buffer.audio_params, &video_buffer.frames, 0);

    //get size to allocate
    // int fps = 1000000.0f/(video_buffer.timer.rate*video_buffer.timer.subdivision);
    int fps         = 15;                          //= 15;
    int samples_per_frame = sample_rate/fps;
    video_buffer.audio_samples_per_frame = samples_per_frame;

    int buff_size = min_buff_len;

    //allocate buffer
    if (!video_buffer.audio_buff) {
        video_buffer.audio_buff = (int16_t*)calloc(buff_size*sizeof(int16_t), 1);
    }

    video_buffer.audio_buff_size = buff_size;
    fill_audio(video_buffer.audio_freq);

    return;
}

void fill_audio(int frequency)
{
    if (video_buffer.audio_buff == NULL) {
        return;
    }
    // Eskemina's example
    int buff_size = video_buffer.audio_buff_size;
    int volume    = video_buffer.audio_volume;
    int channels  = video_buffer.audio_channels;
    int rate      = video_buffer.audio_rate;

    #define PI 3.141592653589793

    switch (channels) {
    case 1:
        for (int i = 0; i < buff_size; i++) {
            double t = (double)i / rate;
            int16_t sample = (int16_t)(sin(2 * PI * frequency * t) * volume);
            video_buffer.audio_buff[i] = sample;
        }
        break;
    case 2:
        for (int i = 0; i < buff_size/channels; i++) {
            double t = (double)i / rate;
            int16_t sample = (int16_t)(sin(2 * PI * frequency * t) * volume);
            video_buffer.audio_buff[i*channels +0] = sample;
            video_buffer.audio_buff[i*channels +1] = sample;
        }
        break;
    default:
        break;
    }
}

struct delta_ch {
    int left = 0;
    int rght = 0;
} last_delta;
delta_ch decompress_16(uint8_t* buff, int len)
{
    int16_t* buff_16 = (int16_t*)buff;
    int16_t l_curr;
    int16_t r_curr;
    int16_t l_last = buff_16[0];
    int16_t r_last = buff_16[1];

    int16_t* audio_buff = video_buffer.audio_buff;
    int audio_buff_size = video_buffer.audio_buff_size;
    int i = 2;
    for (; i < len/2; i++)
    {
        int idx   = i*2;
        int val_l = buff[idx+0];
        int val_r = buff[idx+1];
        l_curr = delta_table[val_l];
        r_curr = delta_table[val_r];

        l_curr += l_last;
        r_curr += r_last;

        audio_buff[idx +0] = l_curr *video_buffer.audio_volume /8;
        audio_buff[idx +1] = r_curr *video_buffer.audio_volume /8;

        l_last = l_curr;
        r_last = r_curr;


    }
    
}

//      lispGamer
//      snd_pcm_status_get_delay    (might be float?)
// Get delay from a PCM status container (see snd_pcm_delay)
// Returns
//     Delay in frames
// Delay is distance between current application frame position and sound frame position. It's positive and less than buffer size in normal situation, negative on playback underrun and greater than buffer size on capture overrun.

//      bakerstaunch
//      snd_pcm_avail_delay
//      snd_pcm_delay
void parse_audio_frame(uint8_t* buffer, opcodeinfo op)
{
    audio_frame* frame = (audio_frame*)buffer;
    //TODO: docs are wrong for frame.length
    //      actual input length is frame.length/(bytes per channel)
    //      actual output length is frame.length

    // video_buffer.audio_calc_rate += frame->length;

    uint8_t decompress_buff[65536] = {0};
    memcpy(decompress_buff, frame->data, op.size-8);
    last_delta = decompress_16(decompress_buff, op.size-8);

    int len = frame->length / 4;  //total number of samples in this chunk
    int16_t* audio_buff = (int16_t*)video_buffer.audio_buff;

    uint32_t tmp;
    snd_pcm_hw_params_get_rate(video_buffer.audio_params, &tmp, 0);
    printf("rate: %d bps file-rate: %d\n", tmp, video_buffer.audio_rate);

    while (len > 0) {

        snd_pcm_sframes_t avail = 0;
        snd_pcm_sframes_t delay = 0;

        int wut = snd_pcm_avail_delay(video_buffer.pcm_handle, &avail, &delay);
        printf("time: %u\t delay: %d\t avail: %d\n", io_nano_time(), delay, avail);

        if (delay > len) {
            break;
        }

        int32_t offset = snd_pcm_writei(video_buffer.pcm_handle, audio_buff, len);

        // printf("time: %u\t offset: %d\t remainder: %d\n", io_nano_time(), offset, len);

        if (offset < 0) {
            if (offset == -EPIPE) {    // Broken pipe (I think this means its not working?)
                printf("Buffer Underrun.\n");
                snd_pcm_prepare(video_buffer.pcm_handle);    //if pipe is broken, then wtf is this doing?
            }
            offset = snd_pcm_recover(video_buffer.pcm_handle, offset, 1);
            if (offset < 0) {
                printf("ALSA Audio ERROR: Can't write to PCM device. %s\n", snd_strerror(offset));
                break;
            }
        }

        audio_buff += offset*2;
        len        -= offset;
    }
}

void shutdown_audio()
{
    snd_pcm_drain(video_buffer.pcm_handle);
    snd_pcm_close(video_buffer.pcm_handle);
    free(video_buffer.audio_buff);
    video_buffer.audio_buff = NULL;
}