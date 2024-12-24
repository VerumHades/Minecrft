#pragma once

#include <rendering/opengl/texture.hpp>
#include <stb_image.h>

#include <unordered_map>
#include <vector>
#include <filesystem>

class TextureRegistry{
    private:
        size_t last_index = 0;

        int texture_width = 0;
        int texture_height = 0;

        GLTextureArray opengl_loaded_textures;

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
        void addTexture(std::string name, std::string path);
        
        /*
            Returns the index of a registered texture
        */
        size_t getTextureIndex(std::string name);
        RegisteredTexture* getTextureByName(std::string name);

        void loadFromFolder(std::string path);

        /*
            Creates the actual opengl object that holds the textures
        */
        void load();

        GLTextureArray& getLoadedTextureArray() { return opengl_loaded_textures; };
};