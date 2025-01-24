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

class BindableTexture{
    protected: 
        uint texture = 0;
        uint TYPE = GL_TEXTURE_2D;
        BindableTexture();
        virtual ~BindableTexture();
    public:
        void bind(int unit) const;
        void unbind(int unit) const;
        void parameter(int identifier, int value);
        uint getType() const;
        uint getID() const;
};

class GLTexture2D: public BindableTexture{
    private:
        bool configured = false;
        void loadData(const Image& image);

    public:
        GLTexture2D(){TYPE = GL_TEXTURE_2D;};
        GLTexture2D(const char* filename);
        GLTexture2D(const Image& image);

        void putImage(int x, int y, Image& image);
        void configure(int internal_format, int format, int data_type, int width, int height, void* data = nullptr, int pixel_pack = 4);
        void reset();
        Image fetch();
};

class GLDepthTexture: public BindableTexture{
    private:
        int width;
        int height;

    public:
        GLDepthTexture(int width, int height);

        int getWidth(){return width;}
        int getHeight(){return height;}
};

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
        void putImage(int x, int y, int layer, Image& image);

        void loadFromFiles(std::vector<std::string>& filenames, int layerWidth, int layerHeight);

        static std::shared_ptr<GLTextureArray> LoadImage(const std::string& path);
};

class GLSkybox: public BindableTexture{
    private:
        uint vertexBufferID;
        uint vao;
    public:
        void load(std::array<std::string, 6> filenames);
        ~GLSkybox();

        void draw();
};

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