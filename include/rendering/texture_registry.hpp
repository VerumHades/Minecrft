#pragma once

#include <rendering/opengl/texture.hpp>
#include <stb_image.h>

#include <unordered_map>
#include <vector>
#include <filesystem>

#include <path_config.hpp>

class TextureRegistry{
    private:
        size_t last_index = 0;

        int texture_width = 0;
        int texture_height = 0;

        struct RegisteredTexture{
            std::string name;
            std::string path;
            size_t index;
        };

        std::unordered_map<std::string, RegisteredTexture> textures;
    public:
        TextureRegistry(): TextureRegistry(64,64) {}
        TextureRegistry(int texture_width, int texture_height): texture_width(texture_width), texture_height(texture_height)  {}

        void setTextureSize(int width, int height){
            texture_width = width;
            texture_height = height;
        }

        /*
            Registers a texture from a path under a name
        */
        void addTexture(std::string name, const std::string& path);
        
        /*
            Returns the index of a registered texture
        */
        size_t getTextureIndex(std::string name);
        RegisteredTexture* getTextureByName(std::string name);

        void loadFromFolder(const std::string& path);

        /*
            Creates the actual opengl object that holds the textures
        */
        std::unique_ptr<GLTextureArray> load();
};