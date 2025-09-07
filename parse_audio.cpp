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
    uint32_t rate, channels, seconds;
    rate     = 48000;
    channels = 2;
    seconds  = 2;

    snd_pcm_t* pcm_handle;
    snd_pcm_hw_params_t* params;
    snd_pcm_uframes_t frames;

    uint8_t* buff;

    pcm = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't open '%s' PCM devicel. %s\n", "default", snd_strerror(pcm));
    }

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);

    //set params
    pcm = snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm));
    }

    pcm = snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set format. %s\n",snd_strerror(pcm));
    }

    pcm = snd_pcm_hw_params_set_channels(pcm_handle, params, channels);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set channels count. %s\n", snd_strerror(pcm));
    }

    pcm = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set rate. %s\n", snd_strerror(pcm));
    }

    //write params
    pcm = snd_pcm_hw_params(pcm_handle, params);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set hardware params. %s\n", snd_strerror(pcm));
    }

    //resume info?
    printf("PCM name: %s\n", snd_pcm_name(pcm_handle));
    printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(pcm_handle)));

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
    printf("seconds: %d (although really what use is this?)\n", seconds);

    // allocate? buffer to hold single period?
    //wait, does this assign something to frames?
    snd_pcm_hw_params_get_period_size(params, &frames, 0);
    //get size to allocate
    int buff_size = frames * channels * 2; //2 is the sample size?
    //allocate buffer
    video_buffer.audio_buff = (uint8_t*)calloc(buff_size, 1);
    uint16_t* audio_buff = (uint16_t*)video_buffer.audio_buff;



    // rate     = 48000;
    // channels = 2;
    // seconds  = 2;

    int middle_C        = 261;  //Hz
    int square_wave_ctr = 0;
    int square_wave_prd = rate/middle_C;
    for (int i = 0; i < buff_size/2; i++)
    {
        // audio_buff[i] = sin(2*pi*frequency*i/sample_rate)*amplitude
        // int16_t sample = (square_wave_ctr > (square_wave_prd/2)) ? 160 : -160;
        // int16_t sample = (int16_t)sin(2*3.1415*middle_C*i/rate)*0;
        // audio_buff[i] = sample;
        audio_buff[i] = 0;

        square_wave_ctr--;
    }


    snd_pcm_hw_params_get_period_time(params, &tmp, NULL);
    for (int loops = (seconds * 1000000); loops > 0; loops--)
    {
        // end of file check here
        //not doing this yet because it should be handled elsewhere
        //if (EOF) {return};

        pcm = snd_pcm_writei(pcm_handle, buff, frames);

        if (pcm == -EPIPE) {    // Broken pipe (I think this means its not working?)
            printf("xRUN.\n");
            snd_pcm_prepare(pcm_handle);    //if pipe is broken, then wtf is this doing?
        } else
        if (pcm < 0) {
            printf("ALSA Audio ERROR: Can't write to PCM device. %s\n", snd_strerror(pcm));
        }


    }


    //close audio stuff?
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    free(video_buffer.audio_buff);




    printf("skipping audio for now\n");
    return;
}

void parse_audio_frame(uint8_t* buffer)
{
    audio_frame frame;
    memcpy(&frame, buffer, sizeof(frame));

    // printf("audio frame - index: %d, mask: %d, length: %d\n", frame.index, frame.mask, frame.length);

}