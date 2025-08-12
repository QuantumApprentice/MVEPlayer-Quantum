#pragma once
#include "shader_class.h"
#include <time.h>

struct mesh {
    GLuint VBO = 0;
    GLuint VAO = 0;
    GLuint EBO = 0;
    GLuint vertexCount = 0;
};

mesh load_giant_triangle();
void blit_to_subtexture(GLuint tex, uint8_t* pxls, int x, int y, int w, int h);
void blit_to_texture(GLuint tex, uint8_t* pxls, int width, int height);
void blit_to_framebuffer(GLuint framebuffer, GLuint texture, Shader* shader, mesh* triangle, int w, int h);