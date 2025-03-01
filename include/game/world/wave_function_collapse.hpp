#pragma once

#include <glm/glm.hpp>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <random>
#include <array>
#include <mutex>

#include <game/blocks.hpp>
#include <vec_hash.hpp>

class Chunk;

namespace WaveFunctionCollapse{
    class ChunkDefinition{
        public:
            enum CrossType {
                WALL,
                OPEN,
                UNKNOWN
            };
        private:
            /* 
            Array that indicates how the chunk connects on all sides: top, bottom, left, right, front, back
            
            This order is depended on, DO NOT CHANGE IT!
            */
            std::array<CrossType,6> neighbours;
        public:
            ChunkDefinition(const std::array<CrossType,6>& available_neighbours): 
                neighbours(available_neighbours) {}

            void place(Chunk* chunk, const glm::ivec3& offset) const;
            const std::array<CrossType,6>& getCrossTypes() {return neighbours;}

            static const ChunkDefinition& getDefinitionFor(const glm::ivec3& position, int seed);

            static const int TOP = 0, BOTTOM = 1, LEFT = 2, RIGHT = 3, FRONT = 4, BACK = 5;
            static const int size = 8; // Size in all directions

            static_assert(64 % size == 0, "Generation chunk size must perfectly align with world chunk size (64).");
    };

};

#include <game/chunk.hpp>