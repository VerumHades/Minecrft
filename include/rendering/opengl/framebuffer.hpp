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

        GLFramebuffer(const GLFramebuffer& other) = delete;
        GLFramebuffer& operator=(const GLFramebuffer& other) = delete;

        GLFramebuffer(GLFramebuffer&& other) noexcept {
            framebuffer_id = other.framebuffer_id;
            depth_renderbuffer_id = other.depth_renderbuffer_id;
            other.framebuffer_id = 0;
            other.depth_renderbuffer_id = 0;
        }

        GLFramebuffer& operator=(GLFramebuffer&& other) noexcept {
            if (this != &other) {
                glDeleteFramebuffers(1, &framebuffer_id); // Delete framebuffer
                glDeleteRenderbuffers(1, &depth_renderbuffer_id); // Delete depth renderbuffer
                framebuffer_id = other.framebuffer_id;
                depth_renderbuffer_id = other.depth_renderbuffer_id;
                other.framebuffer_id = 0;
                other.depth_renderbuffer_id = 0;
            }
            return *this;
        }
        
        void bind();
        void unbind();

        std::vector<GLTexture2D>& getTextures() { return textures; };

        void bindTextures();
        void unbindTextures();

        int getWidth(){return width;}
        int getHeight(){return height;}
};