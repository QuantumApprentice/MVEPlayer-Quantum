#include "parse_audio.h"
#include <memory.h>
#include <math.h>


#include <alsa/asoundlib.h>

#include "parse_video.h"
extern video video_buffer;

struct audio_frame {
    uint16_t index;
    uint16_t mask;
    uint16_t length;
};




void init_audio(uint8_t* chunk)
{
    struct audio_info_v0 {
        int16_t unk;
        int16_t flags;
        int16_t sample_rate;
        int16_t min_buff_len;
    };
    struct audio_info_v1 {
        int16_t unk;
        int16_t flags;
        int16_t sample_rate;
        int32_t min_buff_len;
    };




    uint32_t pcm, tmp, dir;
    // uint32_t rate, channels, seconds;
    // rate     = 48000;
    // channels = 2;
    // seconds  = 2;

    // snd_pcm_t* pcm_handle;
    snd_pcm_hw_params_t* params;
    // snd_pcm_uframes_t frames;

    pcm = snd_pcm_open(&video_buffer.pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't open '%s' PCM devicel. %s\n", "default", snd_strerror(pcm));
    }

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(video_buffer.pcm_handle, params);

    //set params
    pcm = snd_pcm_hw_params_set_access(video_buffer.pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm));
    }

    pcm = snd_pcm_hw_params_set_format(video_buffer.pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set format. %s\n",snd_strerror(pcm));
    }

    pcm = snd_pcm_hw_params_set_channels(video_buffer.pcm_handle, params, video_buffer.channels);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set channels count. %s\n", snd_strerror(pcm));
    }

    pcm = snd_pcm_hw_params_set_rate_near(video_buffer.pcm_handle, params, &video_buffer.rate, 0);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set rate. %s\n", snd_strerror(pcm));
    }

    //write params
    pcm = snd_pcm_hw_params(video_buffer.pcm_handle, params);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set hardware params. %s\n", snd_strerror(pcm));
    }

    //resume info?
    printf("PCM name: %s\n", snd_pcm_name(video_buffer.pcm_handle));
    printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(video_buffer.pcm_handle)));

    //so we _set_ the params earlier
    //  and here we _get_ the params
    //  to make sure they were set?
    snd_pcm_hw_params_get_channels(params, &tmp);
    printf("channels: %i ", tmp);

    if (tmp == 1) {
        printf("(mono)\n");
    } else
    if (tmp == 2) {
        printf("(stereo)\n");
    }

    snd_pcm_hw_params_get_rate(params, &tmp, 0);
    printf("rate: %d bps\n", tmp);
    printf("seconds: %d (although really what use is this?)\n", video_buffer.seconds);

    // allocate? buffer to hold single period?
    //wait, does this assign something to frames?
    snd_pcm_hw_params_get_period_size(params, &video_buffer.frames, 0);
    //get size to allocate
    int buff_size = video_buffer.frames * video_buffer.channels * 2; //2 is the sample size?
    //allocate buffer
    if (!video_buffer.audio_buff) {
        video_buffer.audio_buff = (uint8_t*)calloc(buff_size, 1);
    }
    uint16_t* audio_buff    = (uint16_t*)video_buffer.audio_buff;
    int middle_C        = 261;  //Hz
    int square_wave_ctr = 0;
    int square_wave_prd = video_buffer.rate/middle_C;
    #define PI 3.141592653589793
    // for (int i = 0; i < buff_size/2; i++)
    // {
    //     int16_t sample = (int16_t)(sin(2*PI*middle_C*((double)i/video_buffer.rate))*1600);
    //     audio_buff[i] = sample;
    // }



    // Eskemina's example
    for (int i = 0; i < buff_size/2; i++) {
        double t = (double)i / video_buffer.rate;
        int16_t sample = (int16_t)(sin(2 * PI * middle_C * t) * 1600);
        audio_buff[i] = sample;
    }



    snd_pcm_hw_params_get_period_time(params, &tmp, NULL);

    //close audio stuff?
    // snd_pcm_drain(video_buffer.pcm_handle);
    // snd_pcm_close(video_buffer.pcm_handle);
    // free(video_buffer.audio_buff);


    return;
}

void parse_audio_frame(uint8_t* buffer)
{
    audio_frame frame;
    memcpy(&frame, buffer, sizeof(frame));

    uint16_t* audio_buff    = (uint16_t*)video_buffer.audio_buff;
    // for (int loops = (video_buffer.seconds * 1000000); loops > 0; loops--)
    // {
        // end of file check here
        //not doing this yet because it should be handled elsewhere
        //if (EOF) {return};

        uint32_t pcm = snd_pcm_writei(video_buffer.pcm_handle, audio_buff, video_buffer.frames);

        if (pcm == -EPIPE) {    // Broken pipe (I think this means its not working?)
            printf("xRUN.\n");
            snd_pcm_prepare(video_buffer.pcm_handle);    //if pipe is broken, then wtf is this doing?
        } else
        if (pcm < 0) {
            printf("ALSA Audio ERROR: Can't write to PCM device. %s\n", snd_strerror(pcm));
        }
    // }

    // snd_pcm_drain(video_buffer.pcm_handle);
    // snd_pcm_close(video_buffer.pcm_handle);

}