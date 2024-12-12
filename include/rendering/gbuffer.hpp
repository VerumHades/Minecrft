#pragma once
#include <rendering/opengl/framebuffer.hpp>
#include <rendering/opengl/shaders.hpp>

class GBuffer: public GLFramebuffer{
    public:
        GBuffer(int width, int height);
        void resize(int width, int height);
};