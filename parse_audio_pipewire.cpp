#include "parse_video.h"
#include "parse_opcodes.h"

#include "parse_audio_pipewire.h"

#include <math.h>
#include <pipewire/pipewire.h>
#include <spa/utils/result.h>   //used for error handling
// #include <spa/param/audio/format-utils.h>

extern video video_buffer;


int delta_table_[256] = {
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

void registry_event_global(void* data, uint32_t id,
            uint32_t permissions, const char* type,
            uint32_t version, const struct spa_dict* props);


struct data {
    // int pending;
    struct pw_main_loop* loop;
    struct pw_stream*    stream;
    double accumulator;
};
struct pw_registry_events reg_events = {
    PW_VERSION_REGISTRY_EVENTS,
    .global = registry_event_global
};

// void on_core_done(void* data, uint32_t id, int seq)
// {
//     struct data* d = (struct data*)data;
//     if (id == PW_ID_CORE && seq == d->pending) {
//         pw_main_loop_quit(d->loop);
//     }
// }

// void roundtrip(struct pw_core* core, struct pw_main_loop* loop)
// {
//     static const struct pw_core_events core_events = {
//         PW_VERSION_CORE_EVENTS,
//         .done = on_core_done
//     };
//     struct data d = {.loop = loop};
//     struct spa_hook core_listener;
//     int err;
//     pw_core_add_listener(core, &core_listener, &core_events, &d);
//     d.pending = pw_core_sync(core, PW_ID_CORE, 0);
//     if ((err = pw_main_loop_run(loop)) < 0) {
//         printf("pipewire: main_loop_run error: %d, %s\n", err, spa_strerror(err));
//     }
//     spa_hook_remove(&core_listener);
// }

#define DEFAULT_RATE        44100
#define DEFAULT_CHANNELS    2
#define DEFAULT_VOLUME      0.5
#define DEFAULT_BITS        int16_t

void on_process(void* userdata)
{
    struct data* d = (data*)userdata;
    struct pw_buffer*  pw_b;
    struct spa_buffer* sp_b;
    int i, c, frames, stride;
    DEFAULT_BITS* dst;               //might be assuming 16bit?
    int16_t  val;

    if ((pw_b = pw_stream_dequeue_buffer(d->stream)) == NULL) {
        pw_log_warn("out of buffers: %m");
        return;
    }

    sp_b = pw_b->buffer;
    //might be assuming 16bit?
    if ((dst = (DEFAULT_BITS*)sp_b->datas[0].data) == NULL) {
        return;
    }

    stride = sizeof(DEFAULT_BITS) * DEFAULT_CHANNELS;
    frames = sp_b->datas[0].maxsize/stride;

    

}

void init_audio_pipewire(uint8_t* buff, uint8_t version)
{
    pw_init(0, NULL);

    printf("pipewire initialized\n");
    printf("headers version: %s\n", pw_get_headers_version());
    printf("library version: %s\n", pw_get_library_version());


    struct pw_main_loop* loop;
    struct pw_context*   ctx;
    struct pw_core*      core;
    struct pw_registry*  reg;
    struct spa_hook      reg_listener;

    loop = pw_main_loop_new(NULL /*properties*/);
    ctx  = pw_context_new(
        pw_main_loop_get_loop(loop),
        NULL,       //properties
        0           //user data size
    );

    core = pw_context_connect(
        ctx,
        NULL,       //properties
        0           //user data size
    );

    if (core == NULL) {
        printf("failed to connect pipewire\n");
        return;
    }

    reg = pw_core_get_registry(
        core,
        PW_VERSION_REGISTRY,
        0           //user data size
    );

    // spa_zero(reg_listener);      //why not in part 3?
    pw_registry_add_listener(
        reg,
        &reg_listener,
        &reg_events,
        NULL
    );

    // pw_main_loop_run(loop);
    // roundtrip(core, loop);

    pw_proxy_destroy((struct pw_proxy*)reg);
    pw_core_disconnect(core);
    pw_context_destroy(ctx);
    pw_main_loop_destroy(loop);

}

void registry_event_global(void* data, uint32_t id,
            uint32_t permissions, const char* type,
            uint32_t version, const struct spa_dict* props)
{
    printf("object: id:%u type:%s/%d\n", id, type, version);
}