#pragma once

#include <rendering/opengl/buffer.hpp>
#include <array>

/**
 * @brief A simple quad render for post processing
 * 
 */
class GLFullscreenQuad{
    private:
        GLVertexArray vao = {};
        GLBuffer<float, GL_ARRAY_BUFFER> quad_buffer = {};
    public:
        GLFullscreenQuad();
        void render();
};