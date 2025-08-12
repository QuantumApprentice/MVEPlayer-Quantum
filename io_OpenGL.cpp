#include "io_OpenGL.h"
#include "shader_class.h"

mesh load_giant_triangle()
{
    float vertices[] = {
        //giant triangle     uv coordinates?
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         3.0f, -1.0f, 0.0f,  2.0f, 0.0f,
        -1.0f,  3.0f, 0.0f,  0.0f, 2.0f
    };

    mesh triangle;
    triangle.vertexCount = 3;

    glGenVertexArrays(1, &triangle.VAO);
    glBindVertexArray(triangle.VAO);

    glGenBuffers(1, &triangle.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, triangle.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return triangle;
}

void blit_to_subtexture(GLuint tex, uint8_t* pxls, int x, int y, int w, int h)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    //Change alignment with glPixelStorei() (this change is global/permanent until changed back)
    glPixelStorei(GL_UNPACK_ALIGNMENT, GL_RGB);
    //bind blank background to FRM_texture for display, then paint data onto texture
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RED, GL_UNSIGNED_BYTE, pxls);
}

void blit_to_texture(GLuint tex, uint8_t* pxls, int width, int height)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    //Change alignment with glPixelStorei() (this change is global/permanent until changed back)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    //bind data to FRM_texture for display
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pxls);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void blit_to_framebuffer(GLuint framebuffer, GLuint texture, Shader* shader, mesh* triangle, int w, int h)
{
    glViewport(0, 0, w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glBindVertexArray(triangle->VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    shader->use();
    //draw image to framebuffer
    glDrawArrays(GL_TRIANGLES, 0, triangle->vertexCount);
    //bind framebuffer back to default
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void set_palette(Shader* shader, GLuint* palette)
{
    GLint pal = glGetUniformLocation(shader->ID, "ColorPaletteUINT");
    glUniform1uiv(pal, 256, palette);

    GLenum err = glGetError();
    if (err) {
        printf("draw_texture_to_framebuffer() glGetError: %d\n", err);
        // GL_INVALID_OPERATION
    }

    shader->setInt("Indexed_FRM", 0);
}