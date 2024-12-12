#pragma once

#include <random>
#include <vector>
#include <glm/glm.hpp>
#include <rendering/opengl/texture.hpp>

class GLSSAO{
    private:
        const int kernel_count = 64;
        std::vector<glm::vec3> kernel;
        GLTexture2D noiseTexture;


        float lerp(float a, float b, float f);

    public:
        GLSSAO();
};