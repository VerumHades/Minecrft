#pragma once

#include <glad/glad.h>
#include <general.hpp>
#include <rendering/opengl/texture.hpp>

class GLFramebuffer{
    public:
        struct FramebufferTexture{
            uint internal_format;
            uint format;
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

        std::vector<GLTexture2D>& getTextures() { return textures; };

        void bindTextures();
        void unbindTextures();

        int getWidth(){return width;}
        int getHeight(){return height;}
};