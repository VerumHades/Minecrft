#pragma once

#include <glad/glad.h>
#include <general.hpp>
#include <rendering/opengl/texture.hpp>

/**
 * @brief A frambuffer wrapper for opengl
 * 
 */
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
        ~GLFramebuffer();
        
        // Delete copy constructor and copy assignment
        GLFramebuffer(const GLFramebuffer&)            = delete;
        GLFramebuffer& operator=(const GLFramebuffer&) = delete;

        // Delete move constructor and move assignment
        GLFramebuffer(GLFramebuffer&&)            = delete;
        GLFramebuffer& operator=(GLFramebuffer&&) = delete;
        
        void bind();
        void unbind();

        std::vector<GLTexture2D>& getTextures() { return textures; };

        void bindTextures();
        void unbindTextures();

        int getWidth(){return width;}
        int getHeight(){return height;}
};