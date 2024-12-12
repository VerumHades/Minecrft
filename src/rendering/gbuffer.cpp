#include <rendering/gbuffer.hpp>

GBuffer::GBuffer(int width, int height): 
    GLFramebuffer(width,height, {
        {GL_RGBA16F, GL_FLOAT}, // Positions
        {GL_RGBA16F, GL_FLOAT}, // Normals
        {GL_RGBA,GL_UNSIGNED_BYTE} // Albedo
    }
    ){
        
}

void GBuffer::resize(int width, int height){
    
}