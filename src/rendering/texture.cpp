#include <rendering/texture.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


GLTexture::GLTexture(char* filename){
    int width, height, nrChannels;
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (!data) {
        fprintf(stderr, "Failed to load texture: '%s'.\n", filename);
        return;
    }

    glGenTextures(1, &this->texture);
    glBindTexture(GL_TEXTURE_2D, this->texture);

    CHECK_GL_ERROR();

    // Set texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    float borderColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);  

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    CHECK_GL_ERROR();

    //printf("Texture channels: %i\n", nrChannels);
    // Load image data to GPU
    if (nrChannels == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    } else if (nrChannels == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    } else{
        printf("Invalid number of channels: '%i' in texture image '%s'",nrChannels, filename);
        return;
    }

    CHECK_GL_ERROR();

    glGenerateMipmap(GL_TEXTURE_2D);

    CHECK_GL_ERROR();

    stbi_image_free(data);

    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, texture);
}
void GLTexture::bind(){
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->texture);

    CHECK_GL_ERROR();
}
GLTexture::~GLTexture(){
    glDeleteTextures(1, &this->texture);
}

GLTextureArray::GLTextureArray(){
    glGenTextures(1, &this->textureArray);
}
GLTextureArray::~GLTextureArray(){
    glDeleteTextures(1, &this->textureArray);
}

void GLTextureArray::loadFromFiles(std::vector<std::string> filenames, int layerWidth, int layerHeight){
    glBindTexture(GL_TEXTURE_2D_ARRAY, this->textureArray);

    int mipLevels = floor(log2(fmax(layerWidth, layerHeight))) + 1;
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevels, GL_RGB8, layerWidth, layerHeight, filenames.size());

    int width, height, nrChannels;
    unsigned char *data;  
    for(unsigned int i = 0; i <  filenames.size(); i++)
    {
        data = stbi_load(filenames[i].c_str(), &width, &height, &nrChannels, 0);

        if (!data) {
            throw std::runtime_error("Failed to load texture '%s'\n");
        }

        if(nrChannels == 3){
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, layerWidth, layerHeight, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else if(nrChannels == 4){
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, layerWidth, layerHeight, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else{
            throw std::runtime_error("Invalid number of channels: '%i' in texture image '%s' (4 required).\n");
        }
    }

    CHECK_GL_ERROR();

    // Set texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_REPEAT);

    CHECK_GL_ERROR();

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    CHECK_GL_ERROR();
}

void GLTextureArray::bind(){
    glBindTexture(GL_TEXTURE_2D_ARRAY, this->textureArray);

    CHECK_GL_ERROR();
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

GLSkybox::GLSkybox(std::array<std::string, 6> filenames){
    glGenTextures(1, &this->texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->texture);

    int width, height, nrChannels;
    unsigned char *data;  
    for(unsigned int i = 0; i < 6; i++)
    {
        data = stbi_load(filenames[i].c_str(), &width, &height, &nrChannels, 0);

        if (!data) {
            throw std::runtime_error("Failed to load texture when creating skybox.");
        }


        if(nrChannels == 3){
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else if(nrChannels == 4){
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else{
            throw std::runtime_error("Invalid number of channels in texture image.\n");
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    unsigned int vao;
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_DYNAMIC_DRAW);
    
    CHECK_GL_ERROR();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    this->vertexBuffer = VBO;
    this->vao = vao;
}
void GLSkybox::draw(){
    glDepthMask(GL_FALSE);
    glBindVertexArray(this->vao);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->texture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);
}
GLSkybox::~GLSkybox(){
    glDeleteTextures(1, &this->texture);
    glDeleteBuffers(1 , &this->vertexBuffer);
    glDeleteVertexArrays(1, &this->vao);
}