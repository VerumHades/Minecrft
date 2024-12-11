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

class BindableTexture{
    protected: 
        uint texture = 0;
        uint TYPE = GL_TEXTURE_2D;
        BindableTexture();
        virtual ~BindableTexture();
    public:
        void bind(int unit);
        void unbind(int unit);
        uint getType();
        uint getID();
};

class GLTexture2D: public BindableTexture{
    private:
        bool configured = false;
        void loadData(unsigned char* data, int width, int height, int channels);

    public:
        GLTexture2D(){TYPE = GL_TEXTURE_2D;};
        GLTexture2D(const char* filename);
        GLTexture2D(unsigned char* data, int width, int height);
        void configure(int storage_type, int data_type, int width, int height);
        void reset();
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
    public:
        GLTextureArray();
        void setup(int width, int height, int layers){
            bind(0);
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, width, height,  layers);
        }
        void loadFromFiles(std::vector<std::string> filenames, int layerWidth, int layerHeight);
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

        void addTexture(std::string path);
        std::vector<glm::vec2> getTextureUVs(std::string path);
        int getTextureIndex(std::string path) {return textures[path].index; }

};

#include <rendering/opengl/buffer.hpp>

#endif