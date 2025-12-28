// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp
bool show_demo_window = false;
bool enable_vsync     = true;

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "parse_opcodes.h"
#include "parse_video.h"
#include "parse_audio.h"
#include "ring_buffer.h"

#include "io_Platform.h"
#include "io_Timer.h"
#include <malloc.h>

video video_buffer;
struct Chunk {
    chunkinfo info;
    uint8_t* chunk;
};

FILE* open_file(char* filename);
bool parse_header(FILE* fileptr);
Chunk read_chunk(FILE* fileptr);
bool parse_chunk(Chunk chunk);
int get_filesize(FILE* fileptr);
void video_player();
void show_block_info(ImVec2 pos, float scale);




static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
void init_glad()
{
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        printf("Glad Loader failed?...");
        return;
        // exit(-1);
    } else {
        printf("Vendor: %s\n",       glGetString(GL_VENDOR));
        printf("Renderer: %s\n",     glGetString(GL_RENDERER));
        printf("Version: %s\n",      glGetString(GL_VERSION));
        printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    }
}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

// Main code
int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    // Create window with graphics context
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    GLFWwindow* window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwSetKeyCallback(window, key_callback);
    glfwSetDropCallback(window, file_drop_callback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(enable_vsync); // Enable vsync

    init_glad();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
    // style.Colors[ImGuiCol_WindowBg].w = 1.0f;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    ImVec4 clear_color = ImVec4(0.2f, 0.2f, 0.2f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // ImGui::DockSpaceOverViewport();
#pragma region player_buttons
#ifdef IMGUI_HAS_VIEWPORT
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->GetWorkPos());
        ImGui::SetNextWindowSize(viewport->GetWorkSize());
        ImGui::SetNextWindowViewport(viewport->ID);
#else 
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
#endif

        ImGui::Begin("MVE Player!");
        ImGui::Text("Application average %.3f ms/frame (%.6f FPS)", 1000.0f / io.Framerate, io.Framerate);
        video_player();
        ImGui::End();



        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

FILE* open_file(char* filename)
{
    FILE* fileptr = fopen(filename, "rb");
    if (!fileptr) {
        printf("Failed to load %s\n", filename);
        return NULL;
    }
    return fileptr;
}

bool parse_header(FILE* fileptr)
{
    uint8_t buffer[20];
    fread(buffer, 20, 1, fileptr);

    int comp = memcmp(buffer, "Interplay MVE File\x1A\0", 20);
    if (comp != 0) {
        printf("Not an MVE file.\n");
        return false;
    }
    memset(buffer, 0, 20);

    uint16_t magic[3] = {
        0x001a,
        0x0100,
        0x1133
    };

    for (int i = 0; i < 3; i++)
    {
        fread(buffer, 2, 1, fileptr);
        if (!(uint16_t)*buffer == magic[i]) {
            printf("Failed to match Magic Number: %0xd\n", magic[i]);
            return false;
        }
    }

    return true;
}

int get_filesize(FILE* fileptr)
{
    int curr_pos = ftell(fileptr);
    fseek(fileptr, 0, SEEK_END);
    int file_len = ftell(fileptr);
    fseek(fileptr, curr_pos, SEEK_SET);

    return file_len;
}

bool filter_buttons()
{
    bool* allow_blit     = video_buffer.allow_blit;
    bool* blit_marker    = video_buffer.blit_marker;
#ifdef DEBUG
    bool rerender[0xF+1] = {};
    ImGui::PushItemWidth(100);
    ImGui::SliderInt("data_offset_init", &video_buffer.data_offset_init, -16, 16, NULL);
    ImGui::PopItemWidth();
#endif
    if (ImGui::Button("All Off")) {
        for (int i = 0; i <= 0xF; i++) {
            allow_blit[i] = false;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("All On")) {
        for (int i = 0; i <= 0xF; i++) {
            allow_blit[i] = true;
        }
    }

    for (int i = 0; i <= 0x15; i++)
    {
        ImGui::PushID(i);
        if (i == 1 || i == 6) {
            ImGui::BeginDisabled();
        }
#ifdef DEBUG
        ImGui::PushItemWidth(100);
        rerender[i] = ImGui::SliderInt("offset", &video_buffer.data_offset[i], -16,16, NULL);
        ImGui::PopItemWidth();
        ImGui::SameLine();
#endif

        char button[10];
        snprintf(button, 10, "0x%02x", i);
        if (ImGui::Button(button)) {
            allow_blit[i] = !allow_blit[i];
        }
        ImGui::SameLine();
        ImGui::Text(allow_blit[i] ? "On" : "Off");
        ImGui::SameLine();
        ImGui::Text("op:% 4d", video_buffer.opcode_type[i]);
        if (i <= 0xf) {
            ImGui::SameLine();
            ImGui::Text("en:% 7d", video_buffer.encode_type[i]);
            ImGui::SameLine();
            if (ImGui::Button(blit_marker[i] ? "Unmark" : "Mark")) {
               blit_marker[i] = !blit_marker[i];
            }
        }

        if (i == 1 || i == 6) {
            ImGui::EndDisabled();
        }
        ImGui::PopID();
    }
#ifdef DEBUG
    if (ImGui::Button("reset offsets")) {
        memset(video_buffer.data_offset, 0, (0xF+1)*sizeof(int));
    }
    for (int i = 0; i <= 0xF; i++)
    {
        if (rerender[i]) {
            return true;
        }
    }
#endif

    return false;
}

enum CHUNK {
    CHUNK_init_audio  =  0,
    CHUNK_audio       =  1,
    CHUNK_init_video  =  2,
    CHUNK_video       =  3,
    CHUNK_shutdown    =  4,
    CHUNK_end         =  5,
};

bool load_file(char* filename)
{
    printf("Parsing %s\n", filename);

    if (video_buffer.fileptr) {
        fclose(video_buffer.fileptr);
        video_buffer.fileptr = NULL;
    }
    if (video_buffer.pxls) {
        free(video_buffer.frnt_buffer);
        free(video_buffer.back_buffer);
        video_buffer.pxls        = NULL;
        video_buffer.frnt_buffer = NULL;
        video_buffer.back_buffer = NULL;
    }
    // if (video_buffer.audio && video_buffer.audio->decode_buff) {
        //TODO: what should I do with this?
        //      -I like alsa's behavior with this
        //      but pipewire just seems like ass
        //      -at least I want to restart quick
        //      instead of waiting for buffer to empty
        shutdown_audio();
    // }

    video_buffer.fileptr = open_file(filename);
    if (!video_buffer.fileptr) {
        return false;
    } else {
        video_buffer.file_size = get_filesize(video_buffer.fileptr);
        bool isMVE = parse_header(video_buffer.fileptr);
        if (!isMVE) {
            return false;
        }
        video_buffer.frame_count = 0;
        for (int i = 0; i < 0x15+1; i++)
        {
            if (i < 0x0f+1) {
                video_buffer.encode_type[i] = 0;
            }
            video_buffer.opcode_type[i] = 0;
        }
    }
    return true;
}

void block_select(ImVec2 pos, float scale)
{
    static bool show = false;
    float x_offset = pos.x;
    float y_offset = pos.y;

    ImGuiIO& io = ImGui::GetIO();
    float mouse_x = io.MousePos.x - x_offset;
    float mouse_y = io.MousePos.y - y_offset;

    int block_w      = video_buffer.block_w;
    int block_h      = video_buffer.block_h;
    static int total = 0;

    static uint8_t* selected;
    if (total != block_w*block_h) {
        total = block_w*block_h;
        if (!selected) {
            selected = (uint8_t*)calloc(1, total*sizeof(uint8_t));
        } else {
            free(selected);
            selected = (uint8_t*)calloc(1, total*sizeof(uint8_t));
        }
    }
    for (int y = 0; y < block_h; y++) {
        for (int x = 0; x < block_w; x++) {
            if (x > 0) {ImGui::SameLine();}
            int cur_block = y*block_w + x;
            char num[4];
            snprintf(num, 4, "%03d", cur_block);
            ImGui::PushID(cur_block);
            ImGui::SetCursorPos(ImVec2(x_offset + x*8*scale , y_offset + y*8*scale));
            if (ImGui::Selectable(show ? num : "###", selected[cur_block] != 0, 0, ImVec2(8*scale, 6*scale)))
            {
                // Toggle clicked cell - clear all cells and set single selected
                // if (e == 2) {
                //     memset(selected,0,total);
                //     selected[cur_block] ^= 1;
                // }
            }


            if (mouse_x >= 0 && mouse_y >= 0 && video_buffer.map_stream) {
                int block_num = (int)(mouse_y/(8*scale))*video_buffer.block_w + mouse_x/8/scale;
                int block_enc = video_buffer.map_stream[block_num/2] & (0x0f << ((block_num & 0x01)*4));
                ImGui::SetItemTooltip(
                    "Mouse x: %g y: %g\n"
                    "Block #%d\n"
                    "Encode type: %02x\n"
                    ,
                    mouse_x, mouse_y,
                    block_num, block_enc
                );
            }
            ImGui::PopID();
        }
    }

    ImGui::SetCursorPosX(x_offset);
    if (ImGui::Button("Show Block #")) {
        show = !show;
    }
    show_block_info(pos, scale);

}

//palette can change during playback
void show_palette(ImVec2 pos)
{
    ImGui::SetCursorPos(ImVec2({pos.x, pos.y + 95}));
    for (int y = 0; y < 32; y++)
    {
        ImGui::SetCursorPosX(pos.x+250);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()-3);
        for (int x = 0; x < 8; x++)
        {
            if (x > 0) {ImGui::SameLine(0,0);}
            int index = y*8 + x;
            char color_info[12];
            snprintf(color_info, 12, "%d##aa%d", index, index);
            palette color = video_buffer.pal[index];
            ImGui::ColorButton(color_info, ImVec4(color.r/255.0f, color.g/255.0f, color.b/255.0f, 1.0), 0, ImVec2(18,18));
        }
    }
}

//plots chunks used per frame for easy display
void plot_chunk_usage(ImVec2 pos)
{
    ImGui::SetCursorPos(ImVec2({pos.x+400, pos.y + 10}));
    // Fill an array of contiguous float values to plot
    static float values[101] = {};
    int index = video_buffer.frame_count % 100;
    values[index] = video_buffer.v_chunk_per_frame;
    values[index+1] = 0;
    char overlay[32];
    sprintf(overlay, "Video Chunks per Frame\n         %d", video_buffer.v_chunk_per_frame);
    ImGui::PlotLines("###vcpf", values, 100, 0, overlay, -1.0f, 10.0f, ImVec2(0, 80.0f));
}

//left and right audio channels
void plot_audio_waveform(ImVec2 pos)
{
    ImGui::SetCursorPosX(pos.x+400);
    static float left[4096] = {};
    static float rght[4096] = {};
    int spf = 0;

    if (video_buffer.audio) {
        int16_t* audio_buff = (int16_t*)video_buffer.audio->decode_buff;
        if (audio_buff) {
            for (int i = 0; i < video_buffer.audio->audio_samples_per_frame; i++)
            {
                left[i] = audio_buff[i*2 +0];
                rght[i] = audio_buff[i*2 +1];
            }
        }
        spf = video_buffer.audio->audio_samples_per_frame;
    }


    ImVec2 size = ImGui::GetItemRectSize();
    size.x /= 2;
    ImGui::PlotHistogram("###left",  left, spf, 0, "left", -3000.0f, 3000.0f, size);//ImVec2({400, 80.0f}));
    ImGui::SameLine();
    ImGui::PlotHistogram("###right", rght, spf, 0, "right", -3000.0f, 3000.0f, size);//ImVec2({400, 80.0f}));

}

void show_block_info(ImVec2 pos, float scale)
{
    float x = pos.x;
    float y = pos.y;

    ImGuiIO& io = ImGui::GetIO();
    float mouse_x = io.MousePos.x - x;
    float mouse_y = io.MousePos.y - y;


    if ((io.MousePos.x > x) && io.MousePos.x < (x + video_buffer.render_w*scale)
    &&  (io.MousePos.y > y) && io.MousePos.y < (y + video_buffer.render_h*scale)) {
        mouse_x = io.MousePos.x - pos.x;
        mouse_y = io.MousePos.y - y;
    } else {
        mouse_x = -1;
        mouse_y = -1;
    }

    ImGui::SetCursorPosX(x);
    if (mouse_x >= 0 && mouse_y >= 0 && video_buffer.map_stream) {
        int block_num = (int)(mouse_y/(8*scale))*video_buffer.block_w + mouse_x/8/scale;
        int block_enc = video_buffer.map_stream[block_num/2] & (0x0f << ((block_num & 0x01)*4));
        ImGui::Text(
            "Mouse x: %g y: %g\n"
            "Block #%d\n"
            "Encode type: %02x\n"
            ,
            mouse_x, mouse_y,
            block_num, block_enc
        );
    }  else {
        ImGui::Text("Mouse Out of Bounds");
        ImGui::Dummy({0,ImGui::GetTextLineHeight()*2.35f});
    }
}

void show_audio_info(ImVec2 pos)
{
    float x = pos.x + 260;
    float y = pos.y;
    ImGui::SetCursorPos({x,y});
    if (video_buffer.audio) {
        ImGui::Text(
            "Audio Rate: %dHz\n"
            "Channels: %d %s\n"
            "Bits per Channel:%d\n"
            "Compression %s\n"
            , video_buffer.audio->audio_rate
            , video_buffer.audio->audio_channels, video_buffer.audio->audio_channels == 1 ? "(Mono)" : "(Stereo)"
            , video_buffer.audio->audio_bits
            , video_buffer.audio->audio_compress ? "On" : "Off"
        );
    } else {
        ImGui::Text(
            "Audio Rate:      ---\n"
            "Channels:        ---\n"
            "Bits per Channel: --\n"
            "Compression      ---\n"
        );
    }

    // show amount of audio ring buffer used
    static float vals[101] = {};
    int index = video_buffer.frame_count % 100;
    char total_str[64] = {};
    if (used_ring() > 0 || available_ring() > 0) {
        int used = used_ring();
        int avail = available_ring();
        int total = used + avail;

        snprintf(total_str, 64, "Size %d", total);
        float percent_used = 100*(float)used / (float)(total);
        vals[index] = percent_used;
        vals[index+1] = 0;
    }

    // ImGui::SetCursorPosX(x);
    // ImGui::PlotLines("###buf", vals, 100, 0, total_str, 0.0f, 100.0f, ImVec2(135,35));
    ImGui::SetCursorPos({x+150,y+600});
    ImGui::PlotLines("###buf", vals, 100, 0, total_str, 0.0f, 100.0f, ImVec2(400,100));
}

void plot_fps(ImVec2 pos, float time)
{
    static float values[101] = {};
    int index = video_buffer.frame_count % 100;
    values[index] = time;
    values[index+1] = 0;
    ImGui::PlotLines("###fps", values, 100, 0, "", -1.0f, 20.0f, ImVec2(250, 80.0f));
}

void video_stats()
{
    ImGui::Text(
        "window w: %d  h: %d\n"
        "render w: %d  h: %d\n"
        "block  w: %d  h: %d\n"
        , video_buffer.window_w, video_buffer.window_h
        , video_buffer.render_w, video_buffer.render_h
        , video_buffer.block_w,  video_buffer.block_h
    );
}

void show_video(ImVec2 pos, float scale)
{
    float x = pos.x;
    float y = ImGui::GetCursorPosY();

    ImVec2 uv_min = {0, 0};
    ImVec2 uv_max = {1.0, 1.0};
    int width   = video_buffer.render_w;
    int height  = video_buffer.render_h;

    ImGui::SetCursorPosX(x);
    ImVec2 size = ImVec2((float)(width * scale), (float)(height * scale));
    ImGui::Image(
        video_buffer.video_texture,
        size, uv_min, uv_max
    );

    ImGui::SameLine();
    video_stats();
    block_select({x,y}, scale);
}

struct framerate {
    uint64_t last_time  = 0;
    uint64_t next_frame = 0;
    int64_t diff_time   = 0;
    int32_t frame_time  = 0;
    float speed         = 1.0f;
    timer_struct mve_timer = {};
} fps_info;

void video_player()
{
    uint64_t curr_time = io_nano_time();
    static bool file_loaded     = false;
    static bool render_on_chunk = false;        //TODO: do I want this anymore? currently unused
    ImVec2 pos = ImGui::GetCursorPos();
    static Chunk chunk;
    static int which_file = 0;
    struct file_list { char filename[32]; } files[3] = {
        "../../testing/FO1/IPLOGO.MVE",
        "../../testing/FO2/IPLOGO.MVE",
        "../../testing/final.mve",
    };
    ImGui::PushItemWidth(100);
    ImGui::Combo("audio", &video_buffer.audio_pipe,
        "alsa\0"
        "pipewire\0"
        "pipewire threaded\0"
        "ringbuffer\0"
        "sdl\0"
    );
    ImGui::PopItemWidth();

    char* filename = video_buffer.filename ? video_buffer.filename : files[which_file].filename;
    if (video_buffer.file_drop_frame) {
        video_buffer.file_drop_frame = false;
        file_loaded = load_file(filename);
    }
    if (ImGui::Button("Play MVE")) {
        file_loaded = load_file(filename);
        fps_info.next_frame = curr_time;// fps_info.frame_time / fps_info.speed;
        shutdown_audio();
    }

    if (video_buffer.timer.rate != 0) {
        ImGui::SameLine();
        int rate = video_buffer.timer.rate;
        int subd = video_buffer.timer.subdivision;
        float target_fps = 1000000.0f/(rate*subd);
        ImGui::Text("Target FPS: %f", target_fps);
        ImGui::SetItemTooltip(
            "rate (ms to display frame/subdivision) : %d\n"
            "subdivision (dunno, so far always 8)   : %d\n"
            "FPS = 1,000,000.0f / (rate*subd)", rate, subd
        );
    }

    bool step = false;
    if (ImGui::Button(video_buffer.pause ? "Play" : "Pause")) {
        video_buffer.pause = !video_buffer.pause;
        pause_audio(video_buffer.pause);
    }
    if (video_buffer.pause) {
        ImGui::SameLine();
        if (ImGui::Button("Frame Step")) {
            step = true;
        }
        ImGui::SameLine();
        ImGui::Text("Frame #%d", video_buffer.frame_count);
    }

    // instead of
    //  next_frame = curr_time + frame_time / speed
    // you'd do more
    //  next_time += frame_time / speed
    //(and then adjust them for the correct initialization and handling pauses, etc.)
    // Another approach is
    //  instead of having an absolute time,
    //  have the amount of time remaining until you display the next frame,
    //  then when it hits 0 or goes negative you add the frame time to it

    // fps_info.diff_time = curr_time - fps_info.last_time;
    ImGui::Text("%lld", fps_info.diff_time);
    ImGui::Text("%.3f", 1000.0f/((float)fps_info.diff_time/1'000'000.0));
    // int32_t frame_time = mve_timer.rate*mve_timer.subdivision*1000;

    fps_info.mve_timer = video_buffer.timer;
    fps_info.frame_time = fps_info.mve_timer.rate*fps_info.mve_timer.subdivision*1000;

    plot_fps(pos, 1000.0f/((float)fps_info.diff_time/1'000'000.0));


    if (ImGui::Checkbox("VSync", &enable_vsync)) {
        glfwSwapInterval(enable_vsync); // Enable vsync
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(135);
    ImGui::SliderFloat("Speed", &fps_info.speed, 0.0f, 10.0f, "%.1f");
    ImGui::PopItemWidth();

    ImGui::PushItemWidth(200);
    ImGui::Combo("file", &which_file,
        video_buffer.filename ? video_buffer.filename :
        "FO1 IPLOGO.MVE\0"
        "FO2 IPLOGO.MVE\0"
        "MR  final.mve\0"
    );

    static float scale = 1;                        //video_buffer.scale;
    ImGui::DragFloat("Zoom", &scale, .01, 0, 3.0);


    ImGui::PopItemWidth();
    ImGui::PushItemWidth(100);
    float temp_volume = .5f;
    if (ImGui::DragFloat("V", video_buffer.audio ? &video_buffer.audio->audio_volume : &temp_volume, .001f)) {
        // fill_audio_alsa(audio.audio_freq);
    }
    // ImGui::SameLine();
    // if (ImGui::DragInt("F", &audio.audio_freq)) {
    //     // fill_audio_alsa(audio.audio_freq);
    // }
    ImGui::PopItemWidth();



#ifdef DEBUG
        if (ImGui::Button(video_buffer.encode_bits == 0x0F ? "low order bits first (0x0F)" : "high order bits first (0xF0)")) {
            video_buffer.encode_bits = ~video_buffer.encode_bits;
        }
#endif


    bool rerender = filter_buttons();
    if (ImGui::Button("re-render frame")) {
        rerender = true;
    }
    if (rerender) {
        parse_chunk(chunk);
    }

    show_audio_info(pos);
    show_palette(pos);
    plot_chunk_usage(pos);
    plot_audio_waveform(pos);

    //video
    if (video_buffer.pxls) {
        show_video({pos.x+400,pos.y},scale);
    } else {
        if (!file_loaded) {
            ImGui::SetCursorPosX(pos.x+600);
            ImGui::Text("Unable to open %s", filename);
        }
    }

    if (!file_loaded) {
        ImGui::SetCursorPosX(pos.x+600);
        ImGui::Text("File closed");
    }


    if (!fps_info.speed                     ||
        curr_time <= fps_info.next_frame    ||
        (video_buffer.pause && chunk.info.type == CHUNK_video)) {
        if (step) {
            // bypass the return for one click (should be same as one frame)
        } else {
            return;
        }
    }
    fps_info.next_frame = fps_info.next_frame + (uint64_t)(fps_info.frame_time / fps_info.speed);
    fps_info.diff_time  = curr_time - fps_info.last_time;
    fps_info.last_time  = curr_time;





    while (file_loaded)// && curr_time >= fps_info.next_frame)
    {
        if (chunk.chunk) {
            free(chunk.chunk);
            chunk.chunk = NULL;
        }

        chunk = read_chunk(video_buffer.fileptr);
        bool render_frame = parse_chunk(chunk);
        if (chunk.info.type == CHUNK_end) {
            if (video_buffer.fileptr) {
                fclose(video_buffer.fileptr);
                video_buffer.fileptr = NULL;
            }
            file_loaded = false;
        }

        if (render_frame || render_on_chunk) {
            break;
        }
    }
}



Chunk read_chunk(FILE* fileptr)
{
    Chunk chunk = {
        .chunk = NULL
    };
    if (!fileptr) {
        return chunk;
    }
    int position = ftell(fileptr);
    if (position >= video_buffer.file_size || position < 0) {
        return chunk;
    }


    fread(&chunk.info, sizeof(chunk.info), 1, fileptr);
    chunk.chunk = (uint8_t*)calloc(1, chunk.info.size);
    fread(chunk.chunk, chunk.info.size, 1, fileptr);
    // static int chunk_count = 0;
    // printf("chunk #%d -- size: %d type: %d\n", chunk_count, chunk.info.size, chunk.info.type);

    return chunk;
}

bool parse_chunk(Chunk chunk)
{
    bool render_frame = false;
    switch (chunk.info.type)
    {
    case CHUNK_init_audio:
        printf("initing audio\n");
        // parse_op(chunk.chunk, chunk.info);
        parse_chunk_ops(chunk.chunk, chunk.info);
        // init_audio(chunk.chunk, chunk.info);
        break;
    case CHUNK_audio:
        printf("processing audio\n");
        parse_chunk_ops(chunk.chunk, chunk.info);
        // parse_audio_frame(chunk.chunk);
        //TODO: process the audio
        break;
    case CHUNK_init_video:
        printf("initing video\n");
        parse_chunk_ops(chunk.chunk, chunk.info);
        // init_video(chunk.chunk, chunk.info);
        break;
    case CHUNK_video:
        printf("processing video\n");
        render_frame = parse_video_chunk(chunk.chunk, chunk.info);
        break;
    case CHUNK_shutdown:
        printf("shutting down\n");
        //TODO: handle shutdown
        shutdown_audio();
        break;
    case CHUNK_end:
        printf("end of file\n");
        if (video_buffer.fileptr) {
            fclose(video_buffer.fileptr);
            video_buffer.fileptr = NULL;
        }
        video_buffer.fileptr = NULL;
        //TODO: nothing?
        break;

    default:
        printf("uh oh, we got a missing chunk code or something?\n");
        break;
    }

    // printf("video frame %d processed\n", video_buffer.frame_count);
    return render_frame;
}