
#pragma once

#include <array>
#include <general.hpp>
#include <glm/glm.hpp>

#include <rendering/opengl/buffer.hpp>
#include <rendering/opengl/shaders.hpp>
#include <rendering/image_processing.hpp>
#include <rendering/mesh.hpp>

class CubeRenderer{
    private:
        const size_t maxCubes = 128;
        size_t cubes = 0;

        GLBuffer<float, GL_ARRAY_BUFFER> instanceBuffer;
        std::unique_ptr<LoadedMesh> loaded_mesh;
        std::shared_ptr<GLTexture2D> loaded_texture;

        Uniform<float> texture_count = Uniform<float>("cube_renderer_texture_count");

        ShaderProgram program = ShaderProgram("resources/shaders/graphical/cubes/cube.vs", "resources/shaders/graphical/cubes/cube.fs");
    
    public:
        CubeRenderer();

        void loadTextures(const std::vector<std::string>& textures);

        void setCube(size_t index, glm::vec3 position, int texture);
        void removeCube(size_t index);
        void draw();

        void setCubes(size_t value) {cubes = value;}

        ShaderProgram& getProgram() { return program; };
};