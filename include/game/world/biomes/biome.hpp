#pragma once

#include <game/blocks.hpp>
#include <structure/interval.hpp>

class Structure;

struct StructurePlacement {
    int spawn_chance;
    std::shared_ptr<Structure> structure;
};

struct Biome {
    Interval<float> temperature;
    Interval<float> humidity;

    BlockID surface_block;
    BlockID underground_block;
    BlockID high_block;
    BlockID water_block;

    glm::vec3 preview_color;

    std::vector<StructurePlacement> structures = {};
};
