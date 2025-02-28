#pragma once

#pragma once

#include <scene.hpp>
#include <path_config.hpp>

#include <game/main_scene.hpp>

#include <cctype> // Needed for std::isdigit

class MenuScene: public Scene{
    private:
        
    public:
        void initialize() override;
};
