#include "parse_audio_alsa.h"
#include "parse_video.h"

#include <memory.h>
#include <math.h>


#include <alsa/asoundlib.h>
#include <alsa/control.h>

#include "io_Timer.h"
#include "parse_opcodes.h"

extern video video_buffer;

struct alsa_handle {
    snd_pcm_t*           pcm_handle;
    snd_pcm_hw_params_t* audio_params;
    snd_pcm_uframes_t    frames;
    _snd_pcm_format bitsWide = SND_PCM_FORMAT_U8;
    _snd_pcm_access intrLeav = SND_PCM_ACCESS_RW_INTERLEAVED; //SND_PCM_ACCESS_RW_NONINTERLEAVED; //possibly necessary for mono if there are hardware issues
} alsa;

void init_audio_alsa(audio_handle* audio)
{

    if (audio->audio_bits == 8) {
        alsa.bitsWide = SND_PCM_FORMAT_U8;
    }
    if (audio->audio_bits == 16) {
        alsa.bitsWide = SND_PCM_FORMAT_S16_LE;
    }
    // if (audio->audio_channels == 1) {
    //     alsa.intrLeav = SND_PCM_ACCESS_RW_INTERLEAVED;
    // }
    // if (audio->audio_channels == 2) {
    //     alsa.intrLeav = SND_PCM_ACCESS_RW_INTERLEAVED;
    // }


    int err;
    err = snd_pcm_open(&alsa.pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        printf("ALSA Audio ERROR: Can't open '%s' PCM devicel. %s\n", "default", snd_strerror(err));
        return;
    }

    // snd_pcm_hw_params_alloca(&alsa.audio_params);
    err = snd_pcm_hw_params_malloc(&alsa.audio_params);
    if (err < 0) {
        fprintf (stderr, "ALSA Audio ERROR: Cannot allocate hardware parameter structure (%s)\n",
                snd_strerror (err));
        return;
    }

    //TODO: if this errors randomly then remove the error check
    err = snd_pcm_hw_params_any(alsa.pcm_handle, alsa.audio_params);
    if (err < 0) {
        fprintf (stderr, "ALSA Audio ERROR: Cannot initialize hardware parameter structure (%s)\n",
            snd_strerror(err));
    }

    err = snd_pcm_hw_params_set_format(alsa.pcm_handle, alsa.audio_params, alsa.bitsWide);
    if (err < 0) {
        printf("ALSA Audio ERROR: Can't set format. %s\n",snd_strerror(err));
        return;
    }

    //set params
    err = snd_pcm_hw_params_set_access(alsa.pcm_handle, alsa.audio_params, alsa.intrLeav);
    if (err < 0) {
        printf("ALSA Audio ERROR: Can't set interleaved mode. %s\n", snd_strerror(err));
        return;
    }

    err = snd_pcm_hw_params_set_rate_near(alsa.pcm_handle, alsa.audio_params, &audio->audio_rate, 0);
    if (err < 0) {
        printf("ALSA Audio ERROR: Can't set rate. %s\n", snd_strerror(err));
        return;
    }

    err = snd_pcm_hw_params_set_channels(alsa.pcm_handle, alsa.audio_params, audio->audio_channels);
    if (err < 0) {
        printf("ALSA Audio ERROR: Can't set channels count. %s\n", snd_strerror(err));
        return;
    }

    //write params
    err = snd_pcm_hw_params(alsa.pcm_handle, alsa.audio_params);
    if (err < 0) {
        printf("ALSA Audio ERROR: Can't set hardware params. %s\n", snd_strerror(err));
        return;
    }

    //TODO: docs say this is automatic, but tutorial has it manually https://equalarea.com/paul/alsa-audio.html
    err = snd_pcm_prepare(alsa.pcm_handle);
    if (err < 0) {
        fprintf (stderr, "ALSA Audio ERROR: Cannot prepare audio interface for use (%s)\n",
                snd_strerror (err));
        return;
    }

    //resume info?
    printf("PCM name: %s\n", snd_pcm_name(alsa.pcm_handle));
    printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(alsa.pcm_handle)));

    //so we _set_ the params earlier
    //  and here we _get_ the params
    //  to make sure they were set?
    uint32_t hw_channels;
    snd_pcm_hw_params_get_channels(alsa.audio_params, &hw_channels);
    printf("hw_channels: %i ", hw_channels);

    if (hw_channels == 1) {
        if (audio->audio_channels == 1) {
            printf("(mono)\n");
        } else {
            printf("Error: hardware forced mono\n");
        }
    } else
    if (hw_channels == 2) {
        if (audio->audio_channels == 2) {
            printf("(stereo)\n");
        } else {
            printf("Error: hardware forced stereo\n");
        }
    }
    if (hw_channels > 2) {
        printf("Error: hardware is doing something weird! Channel count: %d\n", hw_channels);
    }

    snd_pcm_hw_params_get_rate(alsa.audio_params, &hw_channels, 0);
    printf("init audio : rate: %d bps file-rate: %d\n", hw_channels, audio->audio_rate);
    // printf("seconds: %d (although really what use is this?)\n", audio->seconds);

    // allocate? buffer to hold single period?
    //wait, does this assign something to frames?
    snd_pcm_hw_params_get_period_size(alsa.audio_params, &alsa.frames, 0);

    // TODO: delete - used for testing
    // fill_audio_alsa(audio->audio_freq);

    return;
}

//TODO: do I want an arbitrary fill for testing?
//      might want to move this to parse_audio.cpp
// Eskemina's example
void fill_audio_alsa(audio_handle* audio)
{
    if (audio->decode_buff == NULL) {
        return;
    }
    int buff_size = audio->decode_buff_size;
    int volume    = audio->audio_volume;
    int channels  = audio->audio_channels;
    int rate      = audio->audio_rate;
    int frequency = audio->audio_freq;

    #define PI 3.141592653589793

    switch (channels) {
    case 1:
        for (int i = 0; i < buff_size; i++) {
            double t = (double)i / rate;
            int16_t sample = (int16_t)(sin(2 * PI * frequency * t) * volume);
            audio->decode_buff[i] = sample;
        }
        break;
    case 2:
        for (int i = 0; i < buff_size/channels; i++) {
            double t = (double)i / rate;
            int16_t sample = (int16_t)(sin(2 * PI * frequency * t) * volume);
            audio->decode_buff[i*channels +0] = sample;
            audio->decode_buff[i*channels +1] = sample;
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
void parse_audio_frame_alsa(audio_handle* audio, uint16_t decode_len)
{
    int16_t* audio_buff = (int16_t*)audio->decode_buff;

    // one audio frame is the total size of one audio sample for each channel
    // for 16-bit stereo that would 2-bytes per channel, or 4-bytes total
    int bytes_per_frame = audio->audio_bits/8 * audio->audio_channels;
    int frame_count = decode_len / bytes_per_frame;

    uint32_t got_rate, got_channels, bits;
    snd_pcm_hw_params_get_rate(alsa.audio_params, &got_rate, 0);
    snd_pcm_hw_params_get_channels(alsa.audio_params, &got_channels);
    bits = snd_pcm_hw_params_get_sbits(alsa.audio_params);
    printf("parsing audio : rate: %d bps file-rate: %d\n", got_rate, audio->audio_rate);
    printf("parsing audio : channels: %d file-channels: %d\n", got_channels, audio->audio_channels);
    printf("parsing audio : bits %d      file-bits: %d\n", bits, audio->audio_bits);

    while (frame_count > 0) {

        snd_pcm_sframes_t avail = 0;
        snd_pcm_sframes_t delay = 0;

        int wut = snd_pcm_avail_delay(alsa.pcm_handle, &avail, &delay);
        printf("time: %u\t delay: %d\t avail: %d\n", io_nano_time(), delay, avail);
        printf("handle: %0x buff[0-3]: %0x length: %d\n", alsa.pcm_handle, *(int32_t*)audio_buff, frame_count);





        int32_t offset = 0;
        if (audio->audio_bits == 8) {
            int8_t* buff_8 = (int8_t*)audio->decode_buff;
            offset = snd_pcm_writei(alsa.pcm_handle, buff_8, frame_count);
        }
        if (audio->audio_bits == 16) {
            offset = snd_pcm_writei(alsa.pcm_handle, audio_buff, frame_count);
        }


        // printf("time: %u\t offset: %d\t remainder: %d\n", io_nano_time(), offset, frame_count);

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
        if (offset != frame_count) {
            fprintf (stderr, "write to audio interface failed (%s)\n",
                snd_strerror (offset));
            return;
        }

        audio_buff  += offset*2;
        frame_count -= offset;
    }
}

void play_alsa(bool pause)
{
    snd_pcm_pause(alsa.pcm_handle, pause);
}

void shutdown_audio_alsa()
{
    snd_pcm_drain(alsa.pcm_handle);
    snd_pcm_close(alsa.pcm_handle);
    snd_pcm_hw_params_free(alsa.audio_params);
    alsa.audio_params = NULL;

}