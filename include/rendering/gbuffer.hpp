#pragma once
#include <rendering/opengl/framebuffer.hpp>
#include <rendering/opengl/shaders.hpp>

class GBuffer: public GLFramebuffer{
    private:
        GLTexture2D positionTexture;
        GLTexture2D normalTexture;
        GLTexture2D albedoTexture;

        GLVertexArray vao;
        GLBuffer<float, GL_ARRAY_BUFFER> quad_buffer;

        int width;
        int height;

        ShaderProgram shader_program = ShaderProgram("shaders/graphical/deffered_shading/gbuffer.vs","shaders/graphical/deffered_shading/gbuffer.fs");
        
    public:
        GBuffer(int width, int height);
        void resize(int width, int height);
        void render();
};