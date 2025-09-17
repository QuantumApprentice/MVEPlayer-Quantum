#include "parse_audio.h"
#include <memory.h>
#include <math.h>


#include <alsa/asoundlib.h>

#include "parse_video.h"
#include "io_Timer.h"

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

    pcm = snd_pcm_hw_params_set_channels(video_buffer.pcm_handle, params, video_buffer.audio_channels);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set channels count. %s\n", snd_strerror(pcm));
    }

    pcm = snd_pcm_hw_params_set_rate_near(video_buffer.pcm_handle, params, &video_buffer.audio_rate, 0);
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
    // int fps = 1000000.0f/(video_buffer.timer.rate*video_buffer.timer.subdivision);
    int fps         = 15;
    int sample_rate = 48000;
    int channels    = 2;
    int video_len   = 180;  //in seconds

    int samples_per_frame = sample_rate/fps;
    int buff_size         = samples_per_frame*channels*2;

    video_buffer.audio_samples_per_frame = samples_per_frame;
    // int channels  = video_buffer.channels;
    //audio sample rate (48kHz) divided by the number of times passing data into alsa per second (fps)
    //  == 48000/15
    // int buff_size = 3200*2*2;

    //allocate buffer
    if (!video_buffer.audio_buff) {
        video_buffer.audio_buff = (int16_t*)calloc(buff_size*sizeof(int16_t), 1);
    }

    // snd_pcm_hw_params_get_period_time(params, &tmp, NULL);

    video_buffer.audio_buff_size = buff_size;
    fill_audio(video_buffer.audio_freq);

    //close audio stuff?
    // snd_pcm_drain(video_buffer.pcm_handle);
    // snd_pcm_close(video_buffer.pcm_handle);
    // free(video_buffer.audio_buff);


    return;
}

void fill_audio(int frequency)
{
    if (video_buffer.audio_buff == NULL) {
        return;
    }
    // Eskemina's example
    // int frequency        = 261;  //Hz //middle C
    int buff_size = video_buffer.audio_buff_size;
    int volume    = video_buffer.audio_volume;
    int channels  = video_buffer.audio_channels;
    #define PI 3.141592653589793
    for (int i = 0; i < buff_size/channels; i++) {
        double t = (double)i / video_buffer.audio_rate;
        int16_t sample = (int16_t)(sin(2 * PI * frequency * t) * volume);
        video_buffer.audio_buff[i*channels +0] = sample;
        video_buffer.audio_buff[i*channels +1] = sample;
    }
}


void parse_audio_frame(uint8_t* buffer)
{
    audio_frame frame;
    memcpy(&frame, buffer, sizeof(frame));

    // long r;
    // int frame_bytes = (snd_pcm_format_physical_width(format) / 8) * channels;
    // while (len > 0) {
    //     r = snd_pcm_writei(handle, buf, len);
    //     if (r == -EAGAIN)
    //         continue;
    //     // printf("write = %li\n", r);
    //     if (r < 0)
    //         return r;
    //     // showstat(handle, 0);
    //     buf += r * frame_bytes;
    //     len -= r;
    //     *frames += r;
    // }

    // int fps = 1000000/(video_buffer.timer.rate * video_buffer.timer.subdivision);



//      lispGamer
//      snd_pcm_status_get_delay    (might be float?)
// Get delay from a PCM status container (see snd_pcm_delay)
// Returns
//     Delay in frames
// Delay is distance between current application frame position and sound frame position. It's positive and less than buffer size in normal situation, negative on playback underrun and greater than buffer size on capture overrun.

//      bakerstaunch
//      snd_pcm_avail_delay
//      snd_pcm_delay

    int16_t* audio_buff = (int16_t*)video_buffer.audio_buff;
    int32_t remainder = video_buffer.audio_samples_per_frame * 2;
    while (remainder > 0) {

        snd_pcm_sframes_t avail = 0;
        snd_pcm_sframes_t delay = 0;

        int wut = snd_pcm_avail_delay(video_buffer.pcm_handle, &avail, &delay);
        printf("time: %u\t delay: %d\t avail: %d\n", io_nano_time(), delay, avail);

        if (delay > 6400) {
            break;
        }

        int32_t offset = snd_pcm_writei(video_buffer.pcm_handle, audio_buff, remainder);


        // printf("time: %u\t offset: %d\t remainder: %d\n", io_nano_time(), offset, remainder);

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
            // continue;
        }


        audio_buff += offset*2;
        remainder  -= offset;
    }


}

void shutdown_audio()
{
    snd_pcm_drain(video_buffer.pcm_handle);
    snd_pcm_close(video_buffer.pcm_handle);
    free(video_buffer.audio_buff);
    video_buffer.audio_buff = NULL;
}