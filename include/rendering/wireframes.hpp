#pragma once

#include <rendering/opengl/buffer.hpp>
#include <array>
#include <general.hpp>
#include <glm/glm.hpp>
#include <rendering/opengl/shaders.hpp>

/**
 * @brief A legacy renderer for cube wireframes
 * 
 */
class WireframeCubeRenderer{
    private:
        GLVertexArray vao;

        const size_t maxCubes = 1024;
        size_t cubes = 0;
        GLBuffer<float, GL_ARRAY_BUFFER        > instanceBuffer;
        GLBuffer<float, GL_ARRAY_BUFFER        > vertexBuffer;
        GLBuffer<uint , GL_ELEMENT_ARRAY_BUFFER> indexBuffer;

        ShaderProgram wireframeProgram = ShaderProgram("resources/shaders/graphical/cubes/cube_wireframe.vs", "resources/shaders/graphical/cubes/cube_wireframe.fs");
    public:
        WireframeCubeRenderer();

        /**
         * @brief Register an instance to be drawn
         * 
         * @param index 
         * @param position 
         * @param scale 
         * @param color 
         */
        void setCube(size_t index, glm::vec3 position, glm::vec3 scale, glm::vec3 color);
        /**
         * @brief Removes  an instance from drawing
         * 
         * @param index 
         */
        void removeCube(size_t index);

        /**
         * @brief Draws all instances
         * 
         */
        void draw();

        /**
         * @brief Set the total amount of cubes to draw
         * 
         * @param value 
         */
        void setCubes(size_t value) {cubes = value;}

        ShaderProgram& getProgram() { return wireframeProgram; };
};