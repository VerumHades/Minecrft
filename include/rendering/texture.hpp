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
        uint texture;
        uint TYPE;
        BindableTexture(){
            glGenTextures(1, &this->texture);
        }
        virtual ~BindableTexture(){
            glDeleteTextures(1, &this->texture);
        }
    public:
        void bind(int unit){
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(TYPE, this->texture);
        }  
        uint getID() {return texture;}
};

class GLTexture: public BindableTexture{
    private:
        void loadData(unsigned char* data, int width, int height, int channels);

    public:
        GLTexture(const char* filename);
        GLTexture(unsigned char* data, int width, int height);
        GLTexture(){TYPE = GL_TEXTURE_2D;};
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

#include <rendering/buffer.hpp>

#endif