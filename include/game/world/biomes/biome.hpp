#pragma once

#include <structure/interval.hpp>
#include <game/blocks.hpp>

struct Biome{
    Interval<float> temperature;
    Interval<float> humidity;

    BlockID surface_block;
    BlockID underground_block;
    BlockID high_block;
    BlockID water_block;

    glm::vec3 preview_color;
};
