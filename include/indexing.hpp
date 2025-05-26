#pragma once

#include <glm/glm.hpp>
#include <array>
#include <vector>
#include <iostream>

/**
 * @brief A class that returns 2D positions in a spiral pattern
 */
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

        /**
         * @brief Returns the current position
         * 
         * @return glm::ivec2 
         */
        glm::ivec2 get();

        /**
         * @brief Steps to the next position
         * 
         */
        void next();

        /**
         * @brief Get the total positions stepped
         * 
         * @return size_t 
         */
        size_t getTotal(){return total;}
};

/**
 * @brief A class that returns 3D positions propagating in spirals from the center point
 * 
 */
class SpiralIndexer3D{
    private:
        int radius = 5;

        int current_distance = 1;
        
        int current_layer = 0;
        int current_layer_direction = 0;

        std::array<std::vector<SpiralIndexer>, 2> layers{};

    public:
        SpiralIndexer3D(){}
        /**
         * @brief Steps to the next position
         * 
         */
        void next();
        /**
         * @brief Returns the current position
         * 
         * @return glm::ivec3 
         */
        glm::ivec3 get();

        /**
         * @brief Get the current furthest distance achieved from center
         * 
         * @return int 
         */
        int getCurrentDistance() { return current_distance; }
};