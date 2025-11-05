#include "parse_audio_alsa.h"
#include "parse_audio.h"

#include <memory.h>
#include <math.h>


#include <alsa/asoundlib.h>

#include "io_Timer.h"
#include "parse_opcodes.h"

extern video video_buffer;

struct alsa_handle {
    snd_pcm_t*           pcm_handle;
    snd_pcm_hw_params_t* audio_params;
    snd_pcm_uframes_t    frames;
    _snd_pcm_format bitsWide = SND_PCM_FORMAT_U8;
    _snd_pcm_access intrLeav = SND_PCM_ACCESS_RW_NONINTERLEAVED;
} alsa;

void init_audio_alsa()
{

    if (video_buffer.audio_channels == 2) {   //0=mono, 1=stereo
        alsa.intrLeav = SND_PCM_ACCESS_RW_INTERLEAVED;
    }
    if (video_buffer.audio_bits == 16) {   //0=8-bit, 1=16-bit
        alsa.bitsWide = SND_PCM_FORMAT_S16_LE;
    }




    int      err;
    err = snd_pcm_open(&alsa.pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        printf("ALSA Audio ERROR: Can't open '%s' PCM devicel. %s\n", "default", snd_strerror(err));
        return;
        // exit (1);
    }

    // snd_pcm_hw_params_alloca(&alsa.audio_params);
    err = snd_pcm_hw_params_malloc(&alsa.audio_params);
    if (err < 0) {
        fprintf (stderr, "ALSA Audio ERROR: Cannot allocate hardware parameter structure (%s)\n",
                snd_strerror (err));
        return;
        // exit (1);
    }

    //TODO: if this errors randomly then remove the error check
    err = snd_pcm_hw_params_any(alsa.pcm_handle, alsa.audio_params);
    if (err < 0) {
        fprintf (stderr, "ALSA Audio ERROR: Cannot initialize hardware parameter structure (%s)\n",
            snd_strerror (err));
    }

    //set params
    err = snd_pcm_hw_params_set_access(alsa.pcm_handle, alsa.audio_params, alsa.intrLeav);
    if (err < 0) {
        printf("ALSA Audio ERROR: Can't set interleaved mode. %s\n", snd_strerror(err));
        return;
        // exit (1);
    }

    err = snd_pcm_hw_params_set_format(alsa.pcm_handle, alsa.audio_params, alsa.bitsWide);
    if (err < 0) {
        printf("ALSA Audio ERROR: Can't set format. %s\n",snd_strerror(err));
        return;
        // exit (1);
    }

    err = snd_pcm_hw_params_set_rate_near(alsa.pcm_handle, alsa.audio_params, &video_buffer.audio_rate, 0);
    if (err < 0) {
        printf("ALSA Audio ERROR: Can't set rate. %s\n", snd_strerror(err));
        return;
        // exit (1);
    }

    err = snd_pcm_hw_params_set_channels(alsa.pcm_handle, alsa.audio_params, video_buffer.audio_channels);
    if (err < 0) {
        printf("ALSA Audio ERROR: Can't set channels count. %s\n", snd_strerror(err));
        return;
        // exit (1);
    }

    //write params
    err = snd_pcm_hw_params(alsa.pcm_handle, alsa.audio_params);
    if (err < 0) {
        printf("ALSA Audio ERROR: Can't set hardware params. %s\n", snd_strerror(err));
        return;
        // exit (1);
    }

    //TODO: docs say this is automatic, but tutorial has it manually https://equalarea.com/paul/alsa-audio.html
    err = snd_pcm_prepare(alsa.pcm_handle);
    if (err < 0) {
        fprintf (stderr, "ALSA Audio ERROR: Cannot prepare audio interface for use (%s)\n",
                snd_strerror (err));
        return;
        // exit (1);
    }

    //resume info?
    printf("PCM name: %s\n", snd_pcm_name(alsa.pcm_handle));
    printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(alsa.pcm_handle)));

    //so we _set_ the params earlier
    //  and here we _get_ the params
    //  to make sure they were set?
    uint32_t tmp;
    snd_pcm_hw_params_get_channels(alsa.audio_params, &tmp);
    printf("channels: %i ", tmp);

    if (tmp == 1) {
        printf("(mono)\n");
    } else
    if (tmp == 2) {
        printf("(stereo)\n");
    }

    snd_pcm_hw_params_get_rate(alsa.audio_params, &tmp, 0);
    printf("init audio : rate: %d bps file-rate: %d\n", tmp, video_buffer.audio_rate);
    // printf("seconds: %d (although really what use is this?)\n", video_buffer.seconds);

    // allocate? buffer to hold single period?
    //wait, does this assign something to frames?
    snd_pcm_hw_params_get_period_size(alsa.audio_params, &video_buffer.frames, 0);

    // fill_audio_alsa(video_buffer.audio_freq);

    return;
}

void fill_audio_alsa(int frequency)
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

//      lispGamer
//      snd_pcm_status_get_delay    (might be float?)
// Get delay from a PCM status container (see snd_pcm_delay)
// Returns
//     Delay in frames
// Delay is distance between current application frame position and sound frame position. It's positive and less than buffer size in normal situation, negative on playback underrun and greater than buffer size on capture overrun.

//      bakerstaunch
//      snd_pcm_avail_delay
//      snd_pcm_delay
void parse_audio_frame_alsa(uint16_t buff_len)
{
    int len = buff_len / 4;  //total number of samples in this chunk
    int16_t* audio_buff = (int16_t*)video_buffer.audio_buff;

    uint32_t tmp;
    snd_pcm_hw_params_get_rate(alsa.audio_params, &tmp, 0);
    printf("parsing audio : rate: %d bps file-rate: %d\n", tmp, video_buffer.audio_rate);

    while (len > 0) {

        snd_pcm_sframes_t avail = 0;
        snd_pcm_sframes_t delay = 0;

        int wut = snd_pcm_avail_delay(alsa.pcm_handle, &avail, &delay);
        printf("time: %u\t delay: %d\t avail: %d\n", io_nano_time(), delay, avail);
        printf("handle: %0x buff[0-3]: %0x length: %d\n", alsa.pcm_handle, *(int32_t*)audio_buff, len);

        //TODO: static bool is temporary fix to get the buffer
        //      to be filled correctly at least for the first frames
        // static bool prepared = false;
        // if (!prepared) {
        //     prepared = true;
        //     //TODO: docs say this is automatic, but tutorial has it manually https://equalarea.com/paul/alsa-audio.html
        //     int err = snd_pcm_prepare(alsa.pcm_handle);
        //     if (err < 0) {
        //         fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
        //                 snd_strerror (err));
        //         return;
        //         // exit (1);
        //     }
        // }

        int32_t offset = 0;
        if (video_buffer.audio_bits == 8) {
            int8_t* buff_8 = (int8_t*)video_buffer.audio_buff;
            offset = snd_pcm_writei(alsa.pcm_handle, buff_8, len);
        }
        if (video_buffer.audio_bits == 16) {
            // if (video_buffer.audio_channels == 1) {
            //     offset = snd_pcm_writen(alsa.pcm_handle, (void**)&audio_buff, len);
            // } else {
                offset = snd_pcm_writei(alsa.pcm_handle, audio_buff, len);
            // }
        }


        // printf("time: %u\t offset: %d\t remainder: %d\n", io_nano_time(), offset, len);

        if (offset < 0) {
            if (offset == -EPIPE) {    // Broken pipe (I think this means its not working?)
                printf("Buffer Underrun.\n");
                snd_pcm_prepare(alsa.pcm_handle);    //if pipe is broken, then wtf is this doing?
            }
            // offset = snd_pcm_recover(alsa.pcm_handle, offset, 1);
            if (offset < 0) {
                printf("ALSA Audio ERROR: Can't write to PCM device. %s\n", snd_strerror(offset));
                break;
            }
        }
        if (offset != len) {
            fprintf (stderr, "write to audio interface failed (%s)\n",
                snd_strerror (offset));
            return;
        }

        audio_buff += offset*2;
        len        -= offset;
    }
}

void shutdown_audio_alsa()
{
    snd_pcm_drain(alsa.pcm_handle);
    snd_pcm_close(alsa.pcm_handle);
    snd_pcm_hw_params_free(alsa.audio_params);
    alsa.audio_params = NULL;

}