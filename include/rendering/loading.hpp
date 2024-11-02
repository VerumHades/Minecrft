#pragma once

#include <glm/glm.hpp>
#include <rendering/buffer.hpp>
#include <rendering/shaders.hpp>
#include <rendering/mesh.hpp>
#include <vec_hash.hpp>
#include <memory>
#include <vector>
#include <unordered_set>

class ChunkLoadingVisualizer{
    private:
        GLBuffer cubeBuffer;
        ShaderProgram program;
        Uniform<glm::mat4> projectionMatrix = Uniform<glm::mat4>("projectionMatrix");
        Uniform<glm::mat4> viewMatrix = Uniform<glm::mat4>("viewMatrix");
        Uniform<glm::mat4> modelMatrix = Uniform<glm::mat4>("modelMatrix");

        std::unordered_set<glm::ivec3, IVec3Hash, IVec3Equal> loadedChunks;

        int rotation = 0;

    public:
        ChunkLoadingVisualizer();

        void addChunk(glm::ivec3 position) {
            if(loadedChunks.count(position) != 0){
                std::cerr << "Loading duplicate?" << std::endl;
            }
            loadedChunks.emplace(position);
        };
        void setup(int screenWidth, int screenHeight);
        void render(int renderDistance);
};