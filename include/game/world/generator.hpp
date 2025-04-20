#pragma once

#include <glm/glm.hpp>
class Chunk;

class Generator {
    public:
        virtual ~Generator() = default;
        virtual void GenerateTerrainChunk(Chunk* chunk, glm::ivec3 position) = 0;
        virtual int GetHeightAt(const glm::vec3 position) = 0;
        virtual void SetSeed(int seed) = 0;
        virtual void Clear() = 0;
};
