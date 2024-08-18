#include <rendering/texture.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.hpp>


GLTexture createTexture(ShaderProgram* program, char* filename){
    GLTexture tex = {0}; 

    int width, height, nrChannels;
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (!data) {
        fprintf(stderr, "Failed to load texture: '%s'.\n", filename);
        return tex;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

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
        return tex;
    }

    CHECK_GL_ERROR();

    glGenerateMipmap(GL_TEXTURE_2D);

    CHECK_GL_ERROR();

    stbi_image_free(data);

    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, texture);

    unsigned int shaderProgram = program->program;
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    tex.texture = texture;
    return tex;
}
void bindTexture(GLTexture* texture){
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->texture);

    CHECK_GL_ERROR();
}
void destroyTexture(GLTexture texture){
    glDeleteTextures(1, &texture.texture);
}

GLTextureArray createTextureArray(ShaderProgram* program){
    GLTextureArray tex = {0}; 

    GLuint textureArray;
    glGenTextures(1, &textureArray);

    tex.program = program->program;
    tex.textureArray = textureArray;
    return tex;
}
void loadTextureArrayFromFiles(GLTextureArray* tex, char* layers[], int layerCount, int layerWidth, int layerHeight){
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex->textureArray);

    int mipLevels = floor(log2(fmax(layerWidth, layerHeight))) + 1;
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevels, GL_RGB8, layerWidth, layerHeight, layerCount);

    int width, height, nrChannels;
    unsigned char *data;  
    for(unsigned int i = 0; i < layerCount; i++)
    {
        data = stbi_load(layers[i], &width, &height, &nrChannels, 0);

        if (!data) {
            fprintf(stderr, "Failed to load texture '%s'\n", layers[i]);
            return;
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
            printf("Invalid number of channels: '%i' in texture image '%s' (4 required).\n",nrChannels, layers[i]);
            stbi_image_free(data);
            return;
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

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->textureArray);
    unsigned int shaderProgram = tex->program;
    glUniform1i(glGetUniformLocation(shaderProgram, "textureArray"), 0);
}

GLTexture3D createTexture3D(ShaderProgram* program){
    GLTexture3D tex = {0}; 

    GLuint texture;
    glGenTextures(1, &texture);

    tex.program = program->program;
    tex.texture = texture;

    return tex;
}
void loadTexture3DRGB(GLTexture3D* tex, unsigned char data[], int layerWidth, int layerHeight, int layerCount){
    glBindTexture(GL_TEXTURE_3D, tex->texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB8, layerWidth, layerCount, layerHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    CHECK_GL_ERROR();

    // Set texture wrapping parameters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    CHECK_GL_ERROR();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, tex->texture);
    unsigned int shaderProgram = tex->program;
    glUniform1i(glGetUniformLocation(shaderProgram, "lightArray"), 1);
}

void updateTexture3DRGB(GLTextureArray* tex, unsigned char data[], int layerWidth, int layerHeight, int layerCount){
    glBindTexture(GL_TEXTURE_3D, tex->textureArray);

    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, layerWidth, layerCount, layerHeight, GL_RGB, GL_UNSIGNED_BYTE, data);

    CHECK_GL_ERROR();
}
void bindTexture3D(GLTexture3D* array){
    glBindTexture(GL_TEXTURE_3D, array->texture);

    CHECK_GL_ERROR();
}
void destroyTexture3D(GLTexture3D* array){
    glDeleteTextures(1, &array->texture);
}

void bindTextureArray(GLTextureArray* array){
    glBindTexture(GL_TEXTURE_2D_ARRAY, array->textureArray);

    CHECK_GL_ERROR();
}
void destroyTextureArray(GLTextureArray* array){
    glDeleteTextures(1, &array->textureArray);
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

GLSkybox createSkybox(char* faces[], int facesTotal){
    GLSkybox skybox = {0};

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    skybox.texture = textureID;

    int width, height, nrChannels;
    unsigned char *data;  
    for(unsigned int i = 0; i < facesTotal; i++)
    {
        data = stbi_load(faces[i], &width, &height, &nrChannels, 0);

        if (!data) {
            fprintf(stderr, "Failed to load texture '%s'\n", faces[i]);
            return skybox;
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
            printf("Invalid number of channels: '%i' in texture image '%s'.\n",nrChannels, faces[i]);
            stbi_image_free(data);
            return skybox;
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

    skybox.vertexBuffer = VBO;
    skybox.vao = vao;

    return skybox;
}
void drawSkybox(GLSkybox* skybox){
    glDepthMask(GL_FALSE);
    glBindVertexArray(skybox->vao);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->texture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);
}
void destroySkybox(GLSkybox* skybox){
    glDeleteTextures(1, &skybox->texture);
    glDeleteBuffers(1 , &skybox->vertexBuffer);
    glDeleteVertexArrays(1, &skybox->vao);
}