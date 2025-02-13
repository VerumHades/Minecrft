#pragma once

#include <glm/glm.hpp>
#include <array>
#include <vector>
#include <iostream>

class SpiralIndexer{
    private:
        size_t total = 0;

        int direction = 0;
        int start_delta = 1;

        glm::ivec2 delta = {1,1};
        glm::ivec2 polarity = {1,1};

        glm::ivec2 current_position = {0,0};
    public:
        SpiralIndexer(){}

        glm::ivec2 get();
        void next();
        size_t getTotal(){return total;}
};

class SpiralIndexer3D{
    private:
        int radius = 5;

        int current_distance = 1;
        
        int current_layer = 0;
        int current_layer_direction = 0;

        std::array<std::vector<SpiralIndexer>, 2> layers{};

    public:
        SpiralIndexer3D(){}
        void next();
        glm::ivec3 get();
        int getCurrentDistance() { return current_distance; }
};