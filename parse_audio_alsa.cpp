#include "parse_audio_alsa.h"
#include "parse_audio.h"

#include <memory.h>
#include <math.h>


#include <alsa/asoundlib.h>

#include "io_Timer.h"
#include "parse_opcodes.h"

extern video video_buffer;

void init_audio_alsa(uint8_t* buff, uint8_t version)
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

    // _snd_pcm_format bitsWide = SND_PCM_FORMAT_U8;
    _snd_pcm_format bitsWide = SND_PCM_FORMAT_S16_LE;
    // _snd_pcm_access intrLeav = SND_PCM_ACCESS_RW_NONINTERLEAVED;
    _snd_pcm_access intrLeav = SND_PCM_ACCESS_RW_INTERLEAVED;
    int channels = 1;   //TODO: might need to experiment when converting mono to stereo
    int bits     = 8;
    int compress = false;
    if (flags & 0x1) {   //0=mono, 1=stereo
        channels = 2;
        intrLeav = SND_PCM_ACCESS_RW_INTERLEAVED;
    }
    if (flags & 0x2) {   //0=8-bit, 1=16-bit
        bits     = 16;
        // bitsWide = SND_PCM_FORMAT_S16_LE;
    }
    if (flags & 0x4) {   //using compression
        compress = true;
    }

    video_buffer.audio_channels = channels;
    video_buffer.audio_rate     = sample_rate;
    video_buffer.audio_bits     = bits;
    video_buffer.audio_compress = compress;



    int      pcm;
    uint32_t tmp;
    // snd_pcm_t* pcm_handle;
    // snd_pcm_hw_params_t* params;
    // snd_pcm_uframes_t frames;

    pcm = snd_pcm_open(&video_buffer.pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't open '%s' PCM devicel. %s\n", "default", snd_strerror(pcm));
        return;
        // exit (1);
    }

    // snd_pcm_hw_params_alloca(&video_buffer.audio_params);
    pcm = snd_pcm_hw_params_malloc(&video_buffer.audio_params);
    if (pcm < 0) {
        fprintf (stderr, "ALSA Audio ERROR: Cannot allocate hardware parameter structure (%s)\n",
                snd_strerror (pcm));
        return;
        // exit (1);
    }

    //TODO: if this errors randomly then remove the error check
    pcm = snd_pcm_hw_params_any(video_buffer.pcm_handle, video_buffer.audio_params);
    if (pcm < 0) {
        fprintf (stderr, "ALSA Audio ERROR: Cannot initialize hardware parameter structure (%s)\n",
            snd_strerror (pcm));
    }

    //set params
    pcm = snd_pcm_hw_params_set_access(video_buffer.pcm_handle, video_buffer.audio_params, intrLeav);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm));
        return;
        // exit (1);
    }

    pcm = snd_pcm_hw_params_set_format(video_buffer.pcm_handle, video_buffer.audio_params, bitsWide);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set format. %s\n",snd_strerror(pcm));
        return;
        // exit (1);
    }

    pcm = snd_pcm_hw_params_set_rate_near(video_buffer.pcm_handle, video_buffer.audio_params, &video_buffer.audio_rate, 0);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set rate. %s\n", snd_strerror(pcm));
        return;
        // exit (1);
    }

    pcm = snd_pcm_hw_params_set_channels(video_buffer.pcm_handle, video_buffer.audio_params, video_buffer.audio_channels);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set channels count. %s\n", snd_strerror(pcm));
        return;
        // exit (1);
    }

    //write params
    pcm = snd_pcm_hw_params(video_buffer.pcm_handle, video_buffer.audio_params);
    if (pcm < 0) {
        printf("ALSA Audio ERROR: Can't set hardware params. %s\n", snd_strerror(pcm));
        return;
        // exit (1);
    }

    //TODO: docs say this is automatic, but tutorial has it manually https://equalarea.com/paul/alsa-audio.html
    pcm = snd_pcm_prepare(video_buffer.pcm_handle);
    if (pcm < 0) {
        fprintf (stderr, "ALSA Audio ERROR: Cannot prepare audio interface for use (%s)\n",
                snd_strerror (pcm));
        return;
        // exit (1);
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
    printf("init audio : rate: %d bps file-rate: %d\n", tmp, video_buffer.audio_rate);
    // printf("seconds: %d (although really what use is this?)\n", video_buffer.seconds);

    // allocate? buffer to hold single period?
    //wait, does this assign something to frames?
    snd_pcm_hw_params_get_period_size(video_buffer.audio_params, &video_buffer.frames, 0);

    //get size to allocate
    // int fps = 1000000.0f/(video_buffer.timer.rate*video_buffer.timer.subdivision);
    int fps         = 15;                          //= 15;
    int samples_per_frame = sample_rate/fps;
    video_buffer.audio_samples_per_frame = samples_per_frame;

    int buff_size;
    if (min_buff_len > 0) {
        buff_size = min_buff_len;
    } else {
        buff_size = samples_per_frame;
    }

    //allocate buffer
    if (!video_buffer.audio_buff) {
        video_buffer.audio_buff = (int16_t*)calloc(buff_size*sizeof(int16_t), 1);
    }

    video_buffer.audio_buff_size = buff_size;
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
void parse_audio_frame_alsa(uint8_t* buffer, opcodeinfo op)
{
    audio_frame* frame = (audio_frame*)buffer;
    //TODO: docs are wrong for frame.length
    //      actual input length is frame.length/(bytes per channel)
    //      actual output length is frame.length

    // video_buffer.audio_calc_rate += frame->length;
    if (video_buffer.audio_compress == 0) {
        // memcpy(video_buffer.audio_buff, frame->data, op.size-8);
        int8_t* audio_buff_8 = (int8_t*)video_buffer.audio_buff;
        if (video_buffer.audio_bits == 8) {
            if (video_buffer.audio_channels == 1) {
                for (int i = 0; i < op.size-8; i++)
                {
                    int8_t sample = frame->data[i];
                    audio_buff_8[i] = sample*video_buffer.audio_volume/8;
                }
            }
            if (video_buffer.audio_channels == 2) {
                for (int i = 0; i < (op.size-8)/2; i++)
                {
                    int8_t l_curr = frame->data[i*2 +0];
                    int8_t r_curr = frame->data[i*2 +1];
                    audio_buff_8[i*2 +0] = l_curr*video_buffer.audio_volume/8;
                    audio_buff_8[i*2 +1] = r_curr*video_buffer.audio_volume/8;
                }
            }
        }
        // for (int i = 0; i < op.size-8; i++)
        // {
        //     int16_t sample = *(int16_t*)frame->data[i];
        //     video_buffer.audio_buff[i] = sample*video_buffer.audio_volume/8;
        // }
    }

    // if (video_buffer.audio_compress == 1) {
        uint8_t decompress_buff[65536] = {0};
        memcpy(decompress_buff, frame->data, op.size-8);
        if (video_buffer.audio_bits == 8 ) {
            decompress_8(decompress_buff, op.size-8);
        }
        if (video_buffer.audio_bits == 16) {
            decompress_16(decompress_buff, op.size-8);
        }
    // }



    int len = frame->length / 4;  //total number of samples in this chunk
    int16_t* audio_buff = (int16_t*)video_buffer.audio_buff;

    uint32_t tmp;
    snd_pcm_hw_params_get_rate(video_buffer.audio_params, &tmp, 0);
    printf("parsing audio : rate: %d bps file-rate: %d\n", tmp, video_buffer.audio_rate);

    while (len > 0) {

        snd_pcm_sframes_t avail = 0;
        snd_pcm_sframes_t delay = 0;

        int wut = snd_pcm_avail_delay(video_buffer.pcm_handle, &avail, &delay);
        printf("time: %u\t delay: %d\t avail: %d\n", io_nano_time(), delay, avail);
        printf("handle: %0x buff[0-3]: %0x length: %d\n", video_buffer.pcm_handle, *(int32_t*)audio_buff, len);

        // if (delay > len) {
        //     break;
        // }

        //TODO: static bool is temporary fix to get the buffer
        //      to be filled correctly at least for the first frames
        // static bool prepared = false;
        // if (!prepared) {
        //     prepared = true;
        //     //TODO: docs say this is automatic, but tutorial has it manually https://equalarea.com/paul/alsa-audio.html
        //     int err = snd_pcm_prepare(video_buffer.pcm_handle);
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
            offset = snd_pcm_writei(video_buffer.pcm_handle, buff_8, len);
        }
        if (video_buffer.audio_bits == 16) {
            // if (video_buffer.audio_channels == 1) {
            //     offset = snd_pcm_writen(video_buffer.pcm_handle, (void**)&audio_buff, len);
            // } else {
                offset = snd_pcm_writei(video_buffer.pcm_handle, audio_buff, len);
            // }
        }


        // printf("time: %u\t offset: %d\t remainder: %d\n", io_nano_time(), offset, len);

        if (offset < 0) {
            if (offset == -EPIPE) {    // Broken pipe (I think this means its not working?)
                printf("Buffer Underrun.\n");
                snd_pcm_prepare(video_buffer.pcm_handle);    //if pipe is broken, then wtf is this doing?
            }
            // offset = snd_pcm_recover(video_buffer.pcm_handle, offset, 1);
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
    snd_pcm_drain(video_buffer.pcm_handle);
    snd_pcm_close(video_buffer.pcm_handle);
    snd_pcm_hw_params_free(video_buffer.audio_params);
    video_buffer.audio_params = NULL;
    free(video_buffer.audio_buff);
    video_buffer.audio_buff = NULL;
}