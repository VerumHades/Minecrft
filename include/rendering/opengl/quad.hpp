#pragma once

#include <rendering/opengl/buffer.hpp>

class GLFullscreenQuad{
    private:
        GLVertexArray vao = {};
        GLBuffer<float, GL_ARRAY_BUFFER> quad_buffer = {};
    public:
        GLFullscreenQuad();
        void render();
};