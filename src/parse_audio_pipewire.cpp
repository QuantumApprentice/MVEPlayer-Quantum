#include "parse_video.h"
#include "parse_opcodes.h"

#include "parse_audio.h"
#include "parse_audio_pipewire.h"
#include "ring_buffer.h"

#include <math.h>
#include <pipewire/pipewire.h>
#include <spa/utils/result.h>   //used for error handling
#include <spa/param/audio/format-utils.h>

extern video video_buffer;
void registry_event_global(void* data, uint32_t id,
            uint32_t permissions, const char* type,
            uint32_t version, const struct spa_dict* props);

#define DEFAULT_CHANNELS    (2)
#define BUFFER_SIZE         (128*1024)
#define DEFAULT_RATE        (44100)
#define DEFAULT_VOLUME      (0.02f)
#define DEFAULT_TONE        (440)
#define DEFAULT_BITS        int16_t

struct pw_data {
    struct pw_main_loop* main_loop;
    struct pw_loop*      loop;
    struct pw_stream*    stream;

    float accumulator;

    struct spa_source* refill_event;

    struct spa_ringbuffer ring;
    // float buff[BUFFER_SIZE * DEFAULT_CHANNELS];
    int16_t buff[BUFFER_SIZE * DEFAULT_CHANNELS];

    // audio_handle* audio;
} pipewire_data = {0};

//fills local buffer with tone generator
void fill_f32(struct pw_data* d, uint32_t offset, int n_frames)
{
    float val;

    for (int i = 0; i < n_frames; i++)
    {
        d->accumulator += 2*M_PI*DEFAULT_TONE / DEFAULT_RATE;
        if (d->accumulator >= 2*M_PI) {
            d->accumulator -= 2*M_PI;   //TODO: set to zero and test
        }

        //same as buff[0+0] || buff[0+1] for both left and right
        val = sinf(d->accumulator) * DEFAULT_VOLUME;
        for (int c = 0; c < DEFAULT_CHANNELS; c++)
        {
            d->buff[((offset + i) % BUFFER_SIZE) * DEFAULT_CHANNELS + c] = val;
        }
    }
}

int fill_from_ring(struct pw_data* d, uint32_t offset, int n_frames)
{
    return copy_from_ring((uint8_t*)&d->buff[offset], n_frames);

}

//fills local buffer from video_buffer.audio->audio_buffer
//stereo only
void fill_int16(struct pw_data* d, uint32_t offset, int n_frames)
{
    for (int i = 0; i < n_frames; i++)
    {
        d->buff[((offset+i)% BUFFER_SIZE) * 2 + 0] = video_buffer.audio->decode_buff[i*2+0];
        d->buff[((offset+i)% BUFFER_SIZE) * 2 + 1] = video_buffer.audio->decode_buff[i*2+1];
    }
}

void do_refill(void* usr_data, uint64_t count)
{
    struct pw_data* data = (struct pw_data*)usr_data;
    int32_t filled;
    uint32_t idx, avail;

    filled = spa_ringbuffer_get_write_index(&data->ring, &idx);
    spa_assert(filled >= 0);
    spa_assert(filled <= BUFFER_SIZE);

    avail = BUFFER_SIZE - filled;

    int actual_read = fill_from_ring(data, idx % BUFFER_SIZE, avail);

    //ringbuffer advance?
    spa_ringbuffer_write_update(&data->ring, idx + actual_read);
    // pw_main_loop_quit(data->main_loop);
}

void on_process(void* userdata)
{
    struct pw_data* d = &pipewire_data;
    struct pw_buffer*  pw_b;
    struct spa_buffer* sp_b;

    uint32_t idx, to_read = 0, to_silence;
    int32_t avail, frames, stride;

    if ((pw_b = pw_stream_dequeue_buffer(d->stream)) == NULL) {
        pw_log_warn("out of buffers: %m");
        return;
    }

    sp_b = pw_b->buffer;
    if (sp_b->datas[0].data == NULL) {  //can't find buffer? shared w/ringbuffer?
        return;
    }

    //returns amount of space in ringbuffer, stores ringbuffer index in idx
    avail = spa_ringbuffer_get_read_index(&d->ring, &idx);

    //TODO: this needs to be able to switch between 8 and 16
    // stride = sizeof(DEFAULT_BITS) * DEFAULT_CHANNELS;
    stride = sizeof(int16_t) * DEFAULT_CHANNELS;
    frames = sp_b->datas[0].maxsize/stride;

    printf("datas[0].maxsize: %d\n", sp_b->datas[0].maxsize);
    printf("frames before requested: %d\n", frames);
    if (pw_b->requested) {
        frames = SPA_MIN(pw_b->requested, frames);
        printf("frames after requested: %d\n", frames);
    }

    //read from the ringbuffer?
    to_read    = avail > 0 ? SPA_MIN(avail, frames) : 0;
    //otherwise silence?
    to_silence = frames - to_read;

    if (to_read > 0) {
        printf("pipewire read %d\n", to_read);
        printf("stride %d, avail %d\n", stride, frames, avail);
        //read data INTO shared datas[0].data buffer from d.buff
        //and apparently datas[0].data is shared with the ringbuffer (or is the ringbuffer?)
        spa_ringbuffer_read_data(
            &d->ring,   //apparently this isn't used in this version anyway
            d->buff,
            BUFFER_SIZE*stride,
            (idx % BUFFER_SIZE) * stride,
            sp_b->datas[0].data,
            to_read*stride
        );
        //update read pointer
        spa_ringbuffer_read_update(&d->ring, idx+to_read);
    }
    if (to_silence > 0) {
        //rest of buffer silenced
        memset(SPA_PTROFF(sp_b->datas[0].data, to_read*stride, void),0,to_silence*stride);
    }

    sp_b->datas[0].chunk->offset = 0;
    sp_b->datas[0].chunk->stride = stride;
    sp_b->datas[0].chunk->size   = frames*stride;

    pw_stream_queue_buffer(d->stream, pw_b);

    /* signal the main thread to fill the ringbuffer,
    * we can only do this, for example
    * when the available ringbuffer space falls
    * below a certain level. */
    pw_loop_signal_event(d->loop, d->refill_event);
}

struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .process = on_process
};
void do_quit(void* userdata, int sig_num)
{
    struct pw_data* d = (pw_data*)userdata;
    pw_main_loop_quit(d->main_loop);
}

//equivalent to main() in pipewire examples
void init_audio_pipewire(audio_handle* audio)
{
    pw_init(0, NULL);
    printf("pipewire initialized\n");
    printf("headers version: %s\n", pw_get_headers_version());
    printf("library version: %s\n", pw_get_library_version());

    struct pw_data* d = &pipewire_data;


    int buff_size = audio->decode_min_buff_len*audio->audio_channels;



    const struct spa_pod* params[1];
    uint8_t pw_audio_buff[1024];
    struct spa_pod_builder pod_b = SPA_POD_BUILDER_INIT(pw_audio_buff,sizeof(pw_audio_buff));

    d->main_loop = pw_main_loop_new(NULL);
    d->loop      = pw_main_loop_get_loop(d->main_loop);

    pw_loop_add_signal(d->loop, SIGINT,  do_quit, d);
    pw_loop_add_signal(d->loop, SIGTERM, do_quit, d);

    spa_ringbuffer_init(&d->ring);
    d->refill_event = pw_loop_add_event(d->loop, do_refill, d);

    struct pw_properties* props = pw_properties_new(
        PW_KEY_MEDIA_TYPE,     "Audio",
        PW_KEY_MEDIA_CATEGORY, "Playback",
        PW_KEY_MEDIA_ROLE,     "Music",
        PW_KEY_NODE_NAME,      "MVE_Player",
        NULL
    );

    d->stream = pw_stream_new_simple(
        d->loop,
        "MVE_Audio",    //not actually displayed as name in pw-top for some reason
        props,
        &stream_events,
        &d
    );

    //TODO: need to handle audio configs per file here?
    struct spa_audio_info_raw audio_info = {
        .format   = SPA_AUDIO_FORMAT_S16,
        .flags    = 0,
        .rate     = audio->audio_rate,
        .channels = audio->audio_channels,
        .position = {0}
    };
    params[0] = spa_format_audio_raw_build(
        &pod_b,
        SPA_PARAM_EnumFormat,
        &audio_info
    );

    pw_stream_connect(
        d->stream,
        PW_DIRECTION_OUTPUT,
        PW_ID_ANY,
        (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                          PW_STREAM_FLAG_MAP_BUFFERS |
                          PW_STREAM_FLAG_RT_PROCESS),
        params, 1
    );

    while (pw_stream_get_state(d->stream, NULL) != PW_STREAM_STATE_STREAMING) {
        pw_loop_iterate(d->loop, 0);
    }

}

void shutdown_audio_pipewire()
{
    struct pw_data* d = &pipewire_data;

    pw_stream_destroy(d->stream);
    pw_loop_destroy_source(d->loop, d->refill_event);
    pw_main_loop_destroy(d->main_loop);
    pw_deinit();
}

void parse_audio_frame_pipewire()
{
    struct pw_data* d = &pipewire_data;

    // pw_main_loop_run(pipewire_data.main_loop);
    pw_loop_iterate(d->loop, -1);
}


void registry_event_global(void* data, uint32_t id,
            uint32_t permissions, const char* type,
            uint32_t version, const struct spa_dict* props)
{
    printf("object: id:%u type:%s/%d\n", id, type, version);
}