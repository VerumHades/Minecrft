#pragma once

#include <game/blocks.hpp>
#include <string>
#include <ui/loader.hpp>
#include <rendering/opengl/shaders.hpp>
#include <algorithm> 
#include <cctype>
#include <locale>

class BlockLoader{
    public:
        static void loadFromSource(std::string source);
        static void loadFromFile(const std::string& path);
};