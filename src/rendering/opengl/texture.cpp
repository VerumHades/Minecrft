#include <rendering/opengl/texture.hpp>
#include <rendering/image_processing.hpp>

static std::array<uint, 32> texture_bindings = {};

BindableTexture::BindableTexture(){
    GL_CALL( glGenTextures(1, &this->texture));
}
BindableTexture::~BindableTexture(){
    GL_CALL( glDeleteTextures(1, &this->texture));
}

static int active_unit = -1;
void BindableTexture::bind(int unit) const{
    if(unit < 0 || unit >= 32) return;
    if(texture_bindings[unit] == this->texture) return;

    //if(active_unit != unit){
        GL_CALL( glActiveTexture(GL_TEXTURE0 + unit));
    //    active_unit = unit;
    //}
    GL_CALL( glBindTexture(TYPE, this->texture));

    texture_bindings[unit] = this->texture;
}  

void BindableTexture::unbind(int unit) const{
    if(unit < 0 || unit >= 32) return;
    if(texture_bindings[unit] != this->texture) return;

    //if(active_unit != unit){
        GL_CALL( glActiveTexture(GL_TEXTURE0 + unit));
    //    active_unit = unit;
    //}
    GL_CALL( glBindTexture(TYPE, 0));

    texture_bindings[unit] = 0;
}

void BindableTexture::parameter(int identifier, int value){
    GL_CALL( glTexParameteri(TYPE, identifier, value));
}

uint BindableTexture::getType() const {return TYPE;}
uint BindableTexture::getID() const {return texture;}

void GLTexture2D::loadData(const Image& image){
    GL_CALL( glBindTexture(GL_TEXTURE_2D, this->texture));

    //CHECK_GL_ERROR();;

    // Set texture wrapping parameters
    GL_CALL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    //CHECK_GL_ERROR();;

    int channels = image.getChannels();

    std::array<GLenum, 4> formats = {
        GL_R8,
        GL_RG,
        GL_RGB,
        GL_RGBA
    };

    GLenum format = formats[channels - 1];
    GL_CALL( glPixelStorei(GL_UNPACK_ALIGNMENT, channels));
    if(!image.getData()){
        LogError("Invalid image being loaded into texture?");
    }
    glTexImage2D(GL_TEXTURE_2D, 0, format, image.getWidth(), image.getHeight(), 0, format, GL_UNSIGNED_BYTE, image.getData());
    GL_CALL( glGenerateMipmap(GL_TEXTURE_2D));
}

GLTexture2D::GLTexture2D(const char* filename){
    TYPE = GL_TEXTURE_2D;

    loadData(Image(filename));
}

GLTexture2D::GLTexture2D(const Image& image){
    TYPE = GL_TEXTURE_2D;

    loadData(image);
}

void GLTexture2D::configure(int storage_type, int color_format, int data_type, int width, int height, void* data, int pixel_pack){
    if(configured) reset();
    bind(0);

    GL_CALL( glPixelStorei(GL_UNPACK_ALIGNMENT, pixel_pack));
    glTexImage2D(GL_TEXTURE_2D, 0, storage_type, width, height, 0, color_format, data_type, data );
    
    parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  

    configured = true;
}

Image GLTexture2D::fetch(){
    bind(0);
    GLint width, height;
    GL_CALL( glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width));
    GL_CALL( glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height));

    GLint internalFormat;
    GL_CALL( glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat));

    int numChannels = 0;
    switch (internalFormat) {
        case GL_R8:          // Single channel (grayscale)
            numChannels = 1;
            break;
        case GL_RGB8:        // 3 channels (RGB)
        case GL_RGB:         // 3 channels (RGB)
            numChannels = 3;
            break;
        case GL_RGBA8:       // 4 channels (RGBA)
        case GL_RGBA:        // 4 channels (RGBA)
            numChannels = 4;
            break;
        case GL_RG8:         // 2 channels (RG)
            numChannels = 2;
            break;
    }

    Image image{width,height,numChannels};
    GL_CALL( glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, (void*) image.getData()));

    return image;
}

void GLTexture2D::putImage(int x, int y, const Image& image){
    bind(0);

    if(!image.getData()){
        LogError("Invalid image being loaded into texture?");
    }
    
    GL_CALL( glPixelStorei(GL_UNPACK_ALIGNMENT, image.getChannels()));
    glTexSubImage2D(
        GL_TEXTURE_2D, // Target
        0,                  // Mipmap level
        x, y,   // x, y, 
        image.getWidth(), image.getHeight(),   // Width, height
        GL_RGBA,            // Format of the pixel data
        GL_UNSIGNED_BYTE,   // Data type of the pixel data
        image.getData()          // Pointer to the image data
    );
}

void GLTexture2D::reset(){
    GL_CALL( glDeleteTextures(1, &this->texture));
    GL_CALL( glGenTextures(1, &this->texture));
}

GLTextureArray::GLTextureArray(){
    TYPE = GL_TEXTURE_2D_ARRAY;
    GL_CALL( glBindTexture(GL_TEXTURE_2D_ARRAY, this->texture));

    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0));

    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_REPEAT));
}

void GLTextureArray::loadFromFiles(std::vector<std::string>& filenames, int layerWidth, int layerHeight){
    TYPE = GL_TEXTURE_2D_ARRAY;
    GL_CALL( glBindTexture(GL_TEXTURE_2D_ARRAY, this->texture));

    layer_width  = layerWidth;
    layer_height = layerHeight;

    int size = (int)filenames.size();

    int mipLevels = (int) floor(log2(fmax(layerWidth, layerHeight))) + 1;
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevels, GL_RGBA8, layerWidth, layerHeight,  size);

    int width = 0, height = 0, nrChannels = 0;
    unsigned char *data;  
    for(int i = 0; i < size; i++)
    {   
        Image texture_image = Image::LoadWithSize(filenames[i], layerWidth, layerHeight);

        if (!texture_image.isLoaded()) {
            LogError("Failed to load texture '{}'", filenames[i]);
            throw std::runtime_error("Failed to load texture '%s'\n");
        }

        putImage(0,0,i, texture_image);
    }

    //CHECK_GL_ERROR();;

    // Set texture wrapping parameters
    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_REPEAT));

    //CHECK_GL_ERROR();;

    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL_CALL( glGenerateMipmap(GL_TEXTURE_2D_ARRAY));

    //CHECK_GL_ERROR();;
}

void GLTextureArray::putImage(int x, int y, int layer, const Image& image){
    bind(0);


    GL_CALL( glPixelStorei(GL_UNPACK_ALIGNMENT, image.getChannels()));
    glTexSubImage3D(
        GL_TEXTURE_2D_ARRAY, 0, 
        x, y, layer, 
        image.getWidth(), image.getHeight(), 1, 
        GL_RGBA, GL_UNSIGNED_BYTE, 
        image.getData()
    );
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

void GLSkybox::load(const std::array<std::string, 6>& filenames){
    TYPE = GL_TEXTURE_CUBE_MAP;

    GL_CALL( glBindTexture(GL_TEXTURE_CUBE_MAP, this->texture));

    int width = 0, height = 0, nrChannels = 0;
    unsigned char *data = nullptr;  
    for(uint i = 0; i < 6; i++)
    {
        data = stbi_load(filenames[i].c_str(), &width, &height, &nrChannels, 0);

        if (!data) {
            LogError("Failed to load texture '{}'", filenames[i]);
            throw std::runtime_error("Failed to load texture when creating skybox.");
        }
       // std::cout << "Loaded " << filenames[i] << " successfully! Channels:" << nrChannels << std::endl;

        if(nrChannels != 4 && nrChannels != 3){
            LogError("Invalid channels in skybox texture: '{}'", nrChannels);
            throw std::runtime_error("Invalid channels in skybox texture.");
        }

        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    GL_CALL( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CALL( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CALL( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));

    uint VBO;
    GL_CALL( glGenBuffers(1, &VBO));
    GL_CALL( glGenVertexArrays(1, &vao));

    GL_CALL( glBindVertexArray(vao));

    GL_CALL( glBindBuffer(GL_ARRAY_BUFFER, VBO));
    GL_CALL( glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_DYNAMIC_DRAW));
    
    //CHECK_GL_ERROR();;

    GL_CALL( glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0));
    GL_CALL( glEnableVertexAttribArray(0));

    GL_CALL( glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL_CALL( glBindVertexArray(0));

    //CHECK_GL_ERROR();;

    this->vertexBufferID = VBO;
    this->vao = vao;
}
void GLSkybox::draw(){
    GL_CALL( glDepthMask(GL_FALSE));
    GL_CALL( glBindVertexArray(this->vao));
    
    //CHECK_GL_ERROR();;
    
    GL_CALL( glBindTexture(GL_TEXTURE_CUBE_MAP, this->texture));
    
    //CHECK_GL_ERROR();;

    GL_CALL( glDrawArrays(GL_TRIANGLES, 0, 36));
    
    //CHECK_GL_ERROR();;
    GL_CALL( glBindVertexArray(0));
    GL_CALL( glDepthMask(GL_TRUE));
}
GLSkybox::~GLSkybox(){
    GL_CALL( glDeleteBuffers(1 , &this->vertexBufferID));
    GL_CALL( glDeleteVertexArrays(1, &this->vao));
}

GLDepthTexture::GLDepthTexture(int width, int height): width(width), height(height){
    TYPE = GL_TEXTURE_2D;

    GL_CALL( glBindTexture(GL_TEXTURE_2D, this->texture));
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    GL_CALL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)); 
    GL_CALL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));  
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GL_CALL( glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor));  
}

DynamicTextureArray::~DynamicTextureArray(){
    for(auto& [name,texture]: textures){
        stbi_image_free(texture.data);
    }
}

void DynamicTextureArray::addTexture(const std::string& path){
    if(textures.count(path) != 0) return;

    GL_CALL( glDeleteTextures(1,&this->texture));
    GL_CALL( glGenTextures(1,&this->texture));
    GL_CALL( glBindTexture(GL_TEXTURE_2D_ARRAY, this->texture));

    /*
        Load the new texture
    */
    int width = 0, height = 0, nrChannels = 0;
    unsigned char *data;  

    data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);

    if (!data) {
        LogError("Failed to load texture '{}'", path);
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
    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_REPEAT));

    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL( glGenerateMipmap(GL_TEXTURE_2D_ARRAY));
}

std::vector<glm::vec2> DynamicTextureArray::getTextureUVs(const std::string& path){
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