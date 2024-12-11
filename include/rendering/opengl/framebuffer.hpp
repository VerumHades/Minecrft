#pragma once

#include <glad/glad.h>
#include <general.hpp>
#include <rendering/opengl/texture.hpp>

class GLFramebuffer{
    public:
        struct FramebufferTexture{
            uint storage_type;
            uint data_type;
        };

    protected:
        int width;
        int height;

    private:
        std::vector<GLTexture2D> textures = {};

        uint framebuffer_id;
        uint depth_renderbuffer_id;
        
    public:
        GLFramebuffer(int width, int height, std::vector<FramebufferTexture> textures);
        void bind();
        void unbind();

        void bindTextures();
        void unbindTextures();
};