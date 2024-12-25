#pragma once

#include <rendering/texture_registry.hpp>
#include <rendering/mesh.hpp>
#include <rendering/model.hpp>
#include <stb_image.h>
#include <memory>
#include <rendering/bitworks.hpp>
#include <unordered_set>
#include <vec_hash.hpp>
#include <bitset>
#include <glm/glm.hpp>

#include <game/blocks.hpp>

class BlockModel: public Model{
    private:
        std::array<glm::vec2, 4> getTextureCoordinates(int i);
    public:
        BlockModel(const BlockRegistry::BlockPrototype* prototype);
};