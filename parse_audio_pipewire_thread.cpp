#include "parse_video.h"
#include "parse_opcodes.h"

#include "parse_audio.h"
#include "parse_audio_pipewire_thread.h"
#include "ring_buffer.h"

#include "io_Timer.h"

#include <math.h>
#include <pipewire/pipewire.h>
#include <spa/utils/result.h>   //used for error handling
#include <spa/param/audio/format-utils.h>

extern video video_buffer;

#define DEFAULT_CHANNELS    (2)
#define BUFFER_SIZE         (128*1024)
#define DEFAULT_RATE        (44100)
#define DEFAULT_VOLUME      (0.02f)
#define DEFAULT_TONE        (440)
#define DEFAULT_BITS        int16_t

struct pw_data {
    struct pw_thread_loop* thread_loop;
    struct pw_loop*      loop;
    struct pw_stream*    stream;
    int eventfd;
    bool running;

    float accumulator;


    struct spa_ringbuffer ring;
    uint16_t buff[BUFFER_SIZE * DEFAULT_CHANNELS];
    int buff_used = 0;

    audio_handle* audio;
} pipewire_data = {0};

void signal_pipewire(uint32_t n_samples)
{
    // struct pw_data* d = &pipewire_data;
    // uint32_t index;
    // // uint32_t filled = spa_ringbuffer_get_write_index(&d->ring, &index);
    // uint32_t avail = BUFFER_SIZE;// - filled;
    // if (avail > n_samples) {avail = n_samples;}
    // spa_ringbuffer_write_update(&d->ring, index + avail);
    // d->buff_used = n_samples;






    struct pw_data* d = &pipewire_data;
    int32_t filled;
    uint32_t index, avail;
    uint32_t stride = sizeof(int16_t) * DEFAULT_CHANNELS;
    uint64_t count;

    // uint8_t* ptr = samples;

    while (n_samples > 0) {
        // while (true) {
        //     // appears to return amount in samples
        //     // the size of which depends on the stride
        //     filled = spa_ringbuffer_get_write_index(&d->ring, &index);
        //     spa_assert(filled >= 0);
        //     spa_assert(filled <= BUFFER_SIZE);

        //     avail = BUFFER_SIZE - filled;
        //     if (avail > 0) {break;}

        //     // if no space then block and wait
        //     spa_system_eventfd_read(d->loop->system, d->eventfd, &count);
        // }
        avail = BUFFER_SIZE - filled;
        if (avail > n_samples) {avail = n_samples;}

        // spa_ringbuffer_write_data(
        //     &d->ring,
        //     d->buff,
        //     BUFFER_SIZE * stride,
        //     (index % BUFFER_SIZE) * stride,
        //     ptr,
        //     avail * stride
        // );

        // ptr += avail * DEFAULT_CHANNELS;
        n_samples -= avail;

        // advance ringbuffer
        spa_ringbuffer_write_update(&d->ring, index + avail);
    }

    // d->buff_used = n_samples;
}

void push_samples(uint8_t* samples, uint32_t n_samples)
{
    struct pw_data* d = &pipewire_data;
    int32_t filled;
    uint32_t index, avail;
    uint32_t stride = sizeof(int16_t) * DEFAULT_CHANNELS;
    uint64_t count;

    uint8_t* ptr = samples;

    while (n_samples > 0) {
        while (true) {
            // appears to return amount in samples
            // the size of which depends on the stride
            filled = spa_ringbuffer_get_write_index(&d->ring, &index);
            spa_assert(filled >= 0);
            spa_assert(filled <= BUFFER_SIZE);

            avail = BUFFER_SIZE - filled;
            if (avail > 0) {break;}

            // if no space then block and wait
            spa_system_eventfd_read(d->loop->system, d->eventfd, &count);
        }
        if (avail > n_samples) {avail = n_samples;}

        spa_ringbuffer_write_data(
            &d->ring,
            d->buff,
            BUFFER_SIZE * stride,
            (index % BUFFER_SIZE) * stride,
            ptr,
            avail * stride
        );

        ptr += avail * DEFAULT_CHANNELS;
        n_samples -= avail;

        // advance ringbuffer
        spa_ringbuffer_write_update(&d->ring, index + avail);
    }

    // d->buff_used = n_samples;
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

    // if (video_buffer.audio_pipe == RINGBUFFER) {
    //     avail = available_ring();
    // }

    //TODO: this needs to be able to switch between 8 and 16
    stride = sizeof(int16_t) * DEFAULT_CHANNELS;
    frames = sp_b->datas[0].maxsize / stride;

    // printf("datas[0].maxsize: %d\n", sp_b->datas[0].maxsize);
    // printf("frames before requested: %d\n", frames);
    if (pw_b->requested) {
        frames = SPA_MIN(pw_b->requested, frames);
        // printf("frames after requested: %d\n", frames);
    }

    //read from the ringbuffer?
    to_read    = avail > 0 ? SPA_MIN(avail, frames) : 0;
    //otherwise silence?
    to_silence = frames - to_read;

    if (to_read > 0) {
        // printf("pipewire read %d\n", to_read);
        // printf("stride %d, avail %d\n", stride, frames, avail);
        //read data INTO shared datas[0].data buffer from d.buff
        //and apparently datas[0].data is shared with the ringbuffer (or is the ringbuffer?)
        spa_ringbuffer_read_data(
            &d->ring,   //apparently this isn't used in this version anyway
            d->buff,
            d->buff_used * stride,
            (idx % BUFFER_SIZE) * stride,
            sp_b->datas[0].data,
            to_read * stride
        );
        //update read pointer
        spa_ringbuffer_read_update(&d->ring, idx+to_read);

        uint64_t time = io_nano_time();
        static uint64_t prev_time;
        io_print_timer("time since last ringbuffer update", prev_time);
        prev_time = time;

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
    // pw_loop_signal_event(d->loop, d->refill_event);
    spa_system_eventfd_write(d->loop->system, d->eventfd, 1);
}

void process_ringbuffer(void* userdata)
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
    // avail = spa_ringbuffer_get_read_index(&d->ring, &idx);
    avail = available_ring();

    //TODO: this needs to be able to switch between 8 and 16
    stride = sizeof(int16_t) * DEFAULT_CHANNELS;
    frames = sp_b->datas[0].maxsize / stride;

    // printf("datas[0].maxsize: %d\n", sp_b->datas[0].maxsize);
    // printf("frames before requested: %d\n", frames);
    if (pw_b->requested) {
        frames = SPA_MIN(pw_b->requested, frames);
        // printf("frames after requested: %d\n", frames);
    }

    //read from the ringbuffer?
    to_read    = avail > 0 ? SPA_MIN(avail, frames) : 0;
    //otherwise silence?
    to_silence = frames - to_read;

    if (to_read > 0) {
        // printf("pipewire read %d\n", to_read);
        // printf("stride %d, avail %d\n", stride, frames, avail);
        // //read data INTO shared datas[0].data buffer from d.buff
        // //and apparently datas[0].data is shared with the ringbuffer (or is the ringbuffer?)
        // spa_ringbuffer_read_data(
        //     &d->ring,   //apparently this isn't used in this version anyway
        //     d->buff,
        //     d->buff_used * stride,
        //     (idx % BUFFER_SIZE) * stride,
        //     sp_b->datas[0].data,
        //     to_read * stride
        // );
        // //update read pointer
        // spa_ringbuffer_read_update(&d->ring, idx+to_read);






        int amt_copied = copy_from_ring((uint8_t*)sp_b->datas[0].data, to_read*stride);
        // if (amt_copied != to_read) {
        //     printf("read attempted: %d succeeded: %d\n");
        // }

        uint64_t time = io_nano_time();
        static uint64_t prev_time;
        io_print_timer("time since last ringbuffer update", prev_time);
        prev_time = time;






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
    // pw_loop_signal_event(d->loop, d->refill_event);
    spa_system_eventfd_write(d->loop->system, d->eventfd, 1);
}

struct pw_stream_events stream_ringbuffer = {
    PW_VERSION_STREAM_EVENTS,
    .process = process_ringbuffer
};

struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .process = on_process
};
void do_quit_thread(void* userdata, int sig_num)
{
    struct pw_data* d = (pw_data*)userdata;
    d->running = false;
}

//equivalent to main() in pipewire examples
void init_audio_pipewire(audio_handle* audio)
{
    pw_init(0, NULL);

    printf("pipewire initialized\n");
    printf("headers version: %s\n", pw_get_headers_version());
    printf("library version: %s\n", pw_get_library_version());

    struct pw_data* d = &pipewire_data;
    d->audio = audio;


    d->thread_loop = pw_thread_loop_new("q_pipewire", NULL);
    d->loop = pw_thread_loop_get_loop(d->thread_loop);
    d->running = true;
    pw_thread_loop_lock(d->thread_loop);

    pw_loop_add_signal(d->loop, SIGINT,  do_quit_thread, d);
    pw_loop_add_signal(d->loop, SIGTERM, do_quit_thread, d);

    spa_ringbuffer_init(&d->ring);
    if ((d->eventfd = spa_system_eventfd_create(d->loop->system, SPA_FD_CLOEXEC)) < 0) {
        // return d->eventfd;   //???????
    }
    pw_thread_loop_start(d->thread_loop);

    struct pw_properties* props = pw_properties_new(
        PW_KEY_MEDIA_TYPE,     "Audio",
        PW_KEY_MEDIA_CATEGORY, "Playback",
        PW_KEY_MEDIA_ROLE,     "Music",
        PW_KEY_NODE_NAME,      "MVE_Player",
        NULL
    );

    if (video_buffer.audio_pipe == RINGBUFFER) {
        d->stream = pw_stream_new_simple(
            d->loop,
            "MVE_Audio",
            props,
            &stream_ringbuffer,
            &d
        );
    } else {
        d->stream = pw_stream_new_simple(
            d->loop,
            "MVE_Audio",    //not actually displayed as name in pw-top for some reason
            props,
            &stream_events,
            &d
        );
    }

    //TODO: need to handle audio configs per file here?
    static struct spa_audio_info_raw audio_info = {
        .format   = SPA_AUDIO_FORMAT_S16,
        .flags    = 0,
        .rate     = audio->audio_rate,
        .channels = audio->audio_channels,
        .position = {0}
    };

    const struct spa_pod* params[1];
    uint8_t pw_audio[1024];
    struct spa_pod_builder pod_b = SPA_POD_BUILDER_INIT(pw_audio, sizeof(pw_audio));
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




    pw_thread_loop_start(d->thread_loop);
    pw_thread_loop_unlock(d->thread_loop);


}

void shutdown_audio_pipewire()
{
    struct pw_data* d = &pipewire_data;

    pw_thread_loop_lock(d->thread_loop);
    pw_stream_destroy(d->stream);
    pw_thread_loop_unlock(d->thread_loop);
    pw_thread_loop_destroy(d->thread_loop);
    close(d->eventfd);
    pw_deinit();
}
