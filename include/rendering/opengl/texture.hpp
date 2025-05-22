#ifndef TEXTURE_3D_H
#define TEXTURE_3D_H

#include <glad/glad.h>
#include <vector>
#include <string>
#include <iostream>
#include <array>
#include <unordered_map>
#include <glm/glm.hpp>
#include <general.hpp>
#include <stb_image.h>
#include <memory>

#include <rendering/image_processing.hpp>

/**
 * @brief A generic bindable texture
 * 
 */
class BindableTexture{
    protected: 
        uint texture = 0;
        uint TYPE = GL_TEXTURE_2D;
        BindableTexture();
        virtual ~BindableTexture();
    public:
        BindableTexture(const BindableTexture& other) = delete;
        BindableTexture& operator=(const BindableTexture& other) = delete;

        BindableTexture(BindableTexture&& other) noexcept {
            texture = other.texture;
            other.texture = 0;
        }

        BindableTexture& operator=(BindableTexture&& other) noexcept {
            if (this != &other) {
                glDeleteTextures(1, &this->texture);
                texture = other.texture;
                other.texture = 0;
            }
            return *this;
        }


        void bind(int unit) const;
        void unbind(int unit) const;
        void parameter(int identifier, int value);
        uint getType() const;
        uint getID() const;
};

/**
 * @brief A simple 2D texture  wrapper
 *  
 */
class GLTexture2D: public BindableTexture{
    private:
        bool configured = false;
        void loadData(const Image& image);

    public:
        GLTexture2D(){TYPE = GL_TEXTURE_2D;};
        GLTexture2D(const char* filename);
        GLTexture2D(const Image& image);

        /**
         * @brief Place image onto the texture
         * 
         * @param x 
         * @param y 
         * @param image 
         */
        void putImage(int x, int y, const Image& image);

        /**
         * @brief Configuration wrapper
         * 
         * @param internal_format 
         * @param format 
         * @param data_type 
         * @param width 
         * @param height 
         * @param data 
         * @param pixel_pack 
         */
        void configure(int internal_format, int format, int data_type, int width, int height, void* data = nullptr, int pixel_pack = 4);
        void reset();
        
        /**
         * @brief Fetch the image back from the gpu
         * 
         * @return Image 
         */
        Image fetch();
};

/**
 * @brief A depth texture wrapper
 * 
 */
class GLDepthTexture: public BindableTexture{
    private:
        int width;
        int height;

    public:
        GLDepthTexture(int width, int height);

        int getWidth(){return width;}
        int getHeight(){return height;}
};

/**
 * @brief An array texture wrapper
 * 
 */
class GLTextureArray: public BindableTexture{
    private:
        int layer_width = 0;
        int layer_height = 0;

    public:
        GLTextureArray();
        void setup(int width, int height, int layers){
            bind(0);
            layer_width = width;
            layer_height = height;
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, width, height,  layers);
        }
        /**
         * @brief Place image onto the texture
         * 
         * @param x 
         * @param y 
         * @param image 
         */
        void putImage(int x, int y, int layer, const Image& image);

        /**
         * @brief Load full texture array from files
         * 
         * @param filenames 
         * @param layerWidth 
         * @param layerHeight 
         */
        void loadFromFiles(std::vector<std::string>& filenames, int layerWidth, int layerHeight);
};

/**
 * @brief A skybox cube texture wrapper
 * 
 */
class GLSkybox: public BindableTexture{
    private:
        uint vertexBufferID = 0;
        uint vao = 0;
    public:
        void load(const std::array<std::string, 6>& filenames);
        GLSkybox(){}
        ~GLSkybox();

        void draw();
};

/**
 * @brief A texture array that images can be appended to and it will automatically resize
 * 
 */
class DynamicTextureArray: public BindableTexture{
    private:
        struct DynamicTextureMember {
            int width;
            int height;
            int index;

            unsigned char* data;
        };
        int rwidth, rheight;

        std::unordered_map<std::string,DynamicTextureMember> textures;

    public:
        DynamicTextureArray() {TYPE = GL_TEXTURE_2D_ARRAY;}
        ~DynamicTextureArray();

        void addTexture(const std::string& path);
        std::vector<glm::vec2> getTextureUVs(const std::string& path);
        int getTextureIndex(const std::string& path) {return textures[path].index; }

};

#include <rendering/opengl/buffer.hpp>

#endif