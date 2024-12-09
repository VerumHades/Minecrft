#pragma once

#include <glad/glad.h>
#include <general.hpp>
#include <rendering/opengl/texture.hpp>

class GLFramebuffer{
    private:
        uint framebuffer_id;
    public:
        GLFramebuffer();
        void bind();
        void unbind();
        void attach(int attachment, GLTexture2D& texture);
};