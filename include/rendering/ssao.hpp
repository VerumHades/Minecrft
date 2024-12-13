#pragma once

#include <random>
#include <vector>
#include <glm/glm.hpp>
#include <rendering/opengl/texture.hpp>
#include <rendering/opengl/framebuffer.hpp>
#include <rendering/opengl/shaders.hpp>
#include <rendering/opengl/quad.hpp>

class GLSSAO{
    private:
        const int kernel_count = 64;
        Uniform<std::vector<glm::vec3>> kernel_uniform = Uniform<std::vector<glm::vec3>>("ssao_samples");
        GLTexture2D noiseTexture;

        GLFramebuffer framebuffer = GLFramebuffer(1920,1080,{{GL_RED,GL_RED,GL_FLOAT}});
        ShaderProgram shader_program = ShaderProgram("shaders/graphical/ssao/ssao.vs","shaders/graphical/ssao/ssao.fs");
        float lerp(float a, float b, float f);

    public:
        GLSSAO();
        void render(GLTexture2D& gPositionTexture, GLTexture2D& gNormalTexture, GLFullscreenQuad& quad);
        GLTexture2D& getResultTexture() { return framebuffer.getTextures()[0]; };
};