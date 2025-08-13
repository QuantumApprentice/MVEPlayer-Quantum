// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "parse_video.h"
#include "parse_audio.h"

FILE* open_file(char* filename);
bool parse_header(FILE* fileptr);
void parse_chunk(FILE* fileptr);
int get_filesize(FILE* fileptr);
void video_player();




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
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

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
    bool show_demo_window = false;
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



        ImGui::Begin("MVE Player!");
        video_player();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
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

void video_player()
{

    static bool success = false;
    static FILE* fileptr = NULL;
    char filename[] = "../../testing/IPLOGO.MVE";

    if (video_buffer.video_buffer) {
        ImVec2 uv_min = {0, 0};
        ImVec2 uv_max = {1.0, 1.0};
        int width   = video_buffer.render_w;
        int height  = video_buffer.render_h;
        float scale = 1;                        //video_buffer.scale;
        ImVec2 size = ImVec2((float)(width * scale), (float)(height * scale));

        ImGui::Image(
            video_buffer.video_texture,
            size, uv_min, uv_max
        );
        // ImGuiWindow *window = ImGui::GetCurrentWindow();
        //TODO: change top_corner() for img_pos passed in from outside
        // window->DrawList->AddImage(
        //     (ImTextureID)(uintptr_t)img_data->render_texture,
        //     top_corner(img_data->offset), bottom_corner(size, top_corner(img_data->offset)),
        //     uv_min, uv_max,
        //     ImGui::GetColorU32(My_Variables->tint_col));
    }

    if (ImGui::Button("Play MVE")) {
        printf("Parsing IPLOGO.MVE\n");

        fileptr = open_file(filename);
        if (!fileptr) {
            success = false;
        }
        video_buffer.file_size = get_filesize(fileptr);
        success = parse_header(fileptr);
    }

    if (success) {
        parse_chunk(fileptr);
    } else {
        // fclose(fileptr);
        ImGui::Text("Unable to open %s", filename);
    }
}

enum CHUNK {
    CHUNK_init_audio  =  0,
    CHUNK_audio       =  1,
    CHUNK_init_video  =  2,
    CHUNK_video       =  3,
    CHUNK_shutdown    =  4,
    CHUNK_end         =  5,
};

#define buffsize (2)
void parse_chunk(FILE* fileptr)
{
    static int frame = 0;
    if (ftell(fileptr) >= video_buffer.file_size) {
        return;
    }
    // while (ftell(fileptr) < size)
    // {
        chunkinfo info;
        uint16_t buffer[buffsize];
        fread(buffer, sizeof(buffer), 1, fileptr);
        info.size = buffer[0];
        info.type = buffer[1];
        // printf("length: %04x : %d\n", info.size, info.size);
        // printf("type:   %04x\n", info.type);

        switch (info.type)
        {
        case CHUNK_init_audio:
            printf("initing audio\n");
            init_audio(fileptr);
            fseek(fileptr, buffer[0], SEEK_CUR);
            //TODO: init the flipping audio
            break;
        case CHUNK_audio:
            printf("skipping processing audio\n");
            fseek(fileptr, buffer[0], SEEK_CUR);
            //TODO: process the audio
            break;
        case CHUNK_init_video:
            printf("initing video\n");
            init_video(fileptr, info);
            break;
        case CHUNK_video:
            printf("processing video\n");
            parse_video_chunk(fileptr, info);
            frame++;
            fseek(fileptr, buffer[0], SEEK_CUR);
            //TODO: process the video
            break;
        case CHUNK_shutdown:
            printf("shutting down\n");
            fseek(fileptr, buffer[0], SEEK_CUR);
            //TODO: handle shutdown
            break;
        case CHUNK_end:
            printf("end of file\n");
            fseek(fileptr, buffer[0], SEEK_CUR);
            fclose(fileptr);
            //TODO: nothing?
            break;

        default:
            printf("uh oh, we got a missing chunk code or something?\n");
            break;
        }

        // fseek(fileptr, buffer[0], SEEK_CUR);
    // }

    printf("%d frames processed\n", frame);


}