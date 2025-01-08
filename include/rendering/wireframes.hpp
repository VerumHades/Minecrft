#pragma once

#include <rendering/opengl/buffer.hpp>
#include <array>
#include <general.hpp>
#include <glm/glm.hpp>
#include <rendering/opengl/shaders.hpp>

class WireframeCubeRenderer{
    private:
        GLVertexArray vao;

        const size_t maxCubes = 2048 * 4;
        size_t cubes = 0;
        GLBuffer<float, GL_ARRAY_BUFFER        > instanceBuffer;
        GLBuffer<float, GL_ARRAY_BUFFER        > vertexBuffer;
        GLBuffer<uint , GL_ELEMENT_ARRAY_BUFFER> indexBuffer;

        ShaderProgram wireframeProgram = ShaderProgram("resources/shaders/graphical/wireframe/cube_wireframe.vs", "resources/shaders/graphical/wireframe/cube_wireframe.fs");
    public:
        WireframeCubeRenderer();

        void setCube(size_t index, glm::vec3 position, glm::vec3 scale, glm::vec3 color);
        void removeCube(size_t index);
        void draw();

        void setCubes(size_t value) {cubes = value;}

        ShaderProgram& getProgram() { return wireframeProgram; };
};