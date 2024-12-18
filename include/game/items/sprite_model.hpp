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

class SpriteModel: public Model{
    private:
        float scale = 1.0f;
        std::string sprite_path;
        
        void setupMesh();
        std::unique_ptr<Mesh> generateFaces();
        glm::ivec4 getPixel(unsigned char* image_data, int x, int y, int width, int height){
            if(x < 0 || y < 0 || x >= width || y >= height) return {0,0,0,0};
            int index = (y * width + x) * 4; // 4 because each pixel is RGBA
            return {
                image_data[index],
                image_data[index + 1],
                image_data[index + 2],
                image_data[index + 3]
            };
        }

    public:
        SpriteModel(std::string sprite_path): sprite_path(sprite_path) {
            setupMesh();
        }
};