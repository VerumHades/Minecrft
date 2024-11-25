#pragma once

#include <rendering/buffer.hpp>
#include <array>
#include <general.hpp>
#include <glm/glm.hpp>
#include <rendering/shaders.hpp>

class WireframeCubeRenderer{
    private:
        GLVertexArray vao;

        const size_t maxCubes = 2048 * 4;
        size_t cubes = 0;
        GLBuffer<float, GL_ARRAY_BUFFER        > instanceBuffer;
        GLBuffer<float, GL_ARRAY_BUFFER        > vertexBuffer;
        GLBuffer<uint , GL_ELEMENT_ARRAY_BUFFER> indexBuffer;

        ShaderProgram wireframeProgram = ShaderProgram("shaders/graphical/wireframe/cube_wireframe.vs", "shaders/graphical/wireframe/cube_wireframe.fs");
    public:
        WireframeCubeRenderer();

        void setCube(size_t index, glm::vec3 position, glm::vec3 scale, glm::vec3 color);
        void removeCube(size_t index);
        void draw();

        ShaderProgram& getProgram() { return wireframeProgram; };
};