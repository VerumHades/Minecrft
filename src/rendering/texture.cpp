#include <rendering/texture.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


void GLTexture::loadData(unsigned char* data, int width, int height, int channels){
    glBindTexture(GL_TEXTURE_2D, this->texture);

    CHECK_GL_ERROR();;

    // Set texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    CHECK_GL_ERROR();;

    //printf("Texture channels: %i\n", nrChannels);
    // Load image data to GPU
    if(channels != 3 && channels != 4){
        std::cout << "Invalid number of channels: " << channels;
        std::terminate();
        return;
    }

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    CHECK_GL_ERROR();;

    glGenerateMipmap(GL_TEXTURE_2D);

    CHECK_GL_ERROR();;
}

GLTexture::GLTexture(const char* filename){
    TYPE = GL_TEXTURE_2D;

    int width = 0, height = 0, nrChannels = 0;
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return;
    }

    loadData(data, width, height, nrChannels);

    stbi_image_free(data);
}

GLTexture::GLTexture(unsigned char* data, int width, int height){
    TYPE = GL_TEXTURE_2D;

    loadData(data, width, height, 4);
}

void GLTextureArray::loadFromFiles(std::vector<std::string> filenames, int layerWidth, int layerHeight){
    TYPE = GL_TEXTURE_2D_ARRAY;
    glBindTexture(GL_TEXTURE_2D_ARRAY, this->texture);

    int size = (int)filenames.size();

    int mipLevels = (int) floor(log2(fmax(layerWidth, layerHeight))) + 1;
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevels, GL_RGBA8, layerWidth, layerHeight,  size);

    int width = 0, height = 0, nrChannels = 0;
    unsigned char *data;  
    for(int i = 0; i < size; i++)
    {   
        std::cout << "Loaded texture: " << filenames[i] << "Channels: " << nrChannels << std::endl;
        data = stbi_load(filenames[i].c_str(), &width, &height, &nrChannels, 4);
    
        if (!data) {
            throw std::runtime_error("Failed to load texture '%s'\n");
        }

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, layerWidth, layerHeight, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    CHECK_GL_ERROR();;

    // Set texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_REPEAT);

    CHECK_GL_ERROR();;

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    CHECK_GL_ERROR();;
}

float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

void GLSkybox::load(std::array<std::string, 6> filenames){
    TYPE = GL_TEXTURE_CUBE_MAP;

    glBindTexture(GL_TEXTURE_CUBE_MAP, this->texture);

    int width = 0, height = 0, nrChannels = 0;
    unsigned char *data = nullptr;  
    for(uint32_t i = 0; i < 6; i++)
    {
        data = stbi_load(filenames[i].c_str(), &width, &height, &nrChannels, 0);

        if (!data) {
            std::cout << "Failed to load texture: " <<  filenames[i] << std::endl;
            throw std::runtime_error("Failed to load texture when creating skybox.");
        }
       // std::cout << "Loaded " << filenames[i] << " successfully! Channels:" << nrChannels << std::endl;

        if(nrChannels != 4 && nrChannels != 3){
            std::cout << "Invalid channels in skybox texture: " << nrChannels << std::endl;
            throw std::runtime_error("Invalid channels in skybox texture.");
        }

        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    uint32_t VBO;
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_DYNAMIC_DRAW);
    
    CHECK_GL_ERROR();;

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    CHECK_GL_ERROR();;

    this->vertexBuffer = VBO;
    this->vao = vao;
}
void GLSkybox::draw(){
    glDepthMask(GL_FALSE);
    glBindVertexArray(this->vao);
    
    CHECK_GL_ERROR();;
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->texture);
    
    CHECK_GL_ERROR();;

    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    CHECK_GL_ERROR();;
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}
GLSkybox::~GLSkybox(){
    glDeleteBuffers(1 , &this->vertexBuffer);
    glDeleteVertexArrays(1, &this->vao);
}

GLDepthTexture::GLDepthTexture(int width, int height): width(width), height(height){
    TYPE = GL_TEXTURE_2D;

    glBindTexture(GL_TEXTURE_2D, this->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);  
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);  
}

DynamicTextureArray::~DynamicTextureArray(){
    for(auto& [name,texture]: textures){
        stbi_image_free(texture.data);
    }
}

void DynamicTextureArray::addTexture(std::string path){
    glDeleteTextures(1,&this->texture);
    glGenTextures(1,&this->texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, this->texture);

    if(textures.count(path) != 0) return;

    /*
        Load the new texture
    */
    int width = 0, height = 0, nrChannels = 0;
    unsigned char *data;  

    data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);

    if (!data) {
        std::cout << "Failed to load texture: " << path << std::endl;
        throw std::runtime_error("Failed to load texture.\n");
    }

    textures[path] = {
        width,
        height,
        0,
        data
    };
    /*
        Update the texture array
    */

    for(auto& [path, texture]: textures){
        rwidth = std::max(texture.width, rwidth);
        rheight = std::max(texture.height, rheight);
    }

    int textureCount = textures.size();

    int mipLevels = (int) floor(log2(fmax(rwidth, rheight))) + 1;
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevels, GL_RGB8, rwidth, rheight, textureCount);

    
    int i = 0;
    for(auto& [path, texture]: textures)
    {   
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, rwidth, rheight, 1, GL_RGBA, GL_UNSIGNED_BYTE, texture.data);
        texture.index = i;
        i++;
    }

    // Doesnt really need to be done every time
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
}

std::vector<glm::vec2> DynamicTextureArray::getTextureUVs(std::string path){
    if(textures.count(path) == 0) {};

    DynamicTextureMember& tex = textures.at(path);

    float deltaX = (float)tex.width  / (float)rwidth ;
    float deltaY = (float)tex.height / (float)rheight;

    return {
        {
            {0     , 0     },
            {deltaX, 0     },
            {deltaX, deltaY},
            {0     , deltaY}
        }
    };
}