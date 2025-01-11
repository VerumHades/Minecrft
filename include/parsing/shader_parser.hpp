#pragma once

#include <glad/glad.h>
#include <parsing/tokenizer.hpp>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <general.hpp>

class ShaderProgramSource{
    private:
        struct ShaderSource{
            uint shader_type;
            std::string source;
        };
        std::vector<ShaderSource> shader_sources;

        ShaderProgramSource(std::string source);

    public:
        static ShaderProgramSource fromFile(const std::string& path);
        static ShaderProgramSource fromSource(std::string source);

        std::vector<ShaderSource>& getSources() {return shader_sources;};

};