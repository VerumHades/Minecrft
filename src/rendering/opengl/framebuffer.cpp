#include <rendering/opengl/framebuffer.hpp>

static uint currently_bound = 0;
GLFramebuffer::GLFramebuffer(int width, int height, std::vector<FramebufferTexture> texture_definitions): width(width), height(height){
    glGenFramebuffers(1, &framebuffer_id);
    bind();

    glGenRenderbuffers(1, &depth_renderbuffer_id);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer_id);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer_id);

    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    size_t textures_total = texture_definitions.size();
    textures.resize(textures_total);

    std::vector<uint> attachments(textures_total);
    for(int i = 0; i < textures.size();i++){
        auto& definition = texture_definitions[i];
        auto& texture = textures[i];

        texture.configure(definition.storage_type, definition.data_type, width, height);

        attachments[i] = GL_COLOR_ATTACHMENT0 + i;
        
        glFramebufferTexture2D(GL_FRAMEBUFFER,  attachments[i] , GL_TEXTURE_2D, texture.getID(), 0);
    }

    glDrawBuffers(textures_total, attachments.data());

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) 
        throw std::runtime_error("Failed to create framebuffer!");

    unbind();
}
void GLFramebuffer::bind(){
    if(currently_bound == framebuffer_id) return;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
    currently_bound = framebuffer_id;
}
void GLFramebuffer::unbind(){
    if(currently_bound == 0) return;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    currently_bound = 0;
}

void GLFramebuffer::bindTextures(){
    for(int i = 0;i < textures.size();i++) textures[i].bind(i);
}
void GLFramebuffer::unbindTextures(){
    for(int i = 0;i < textures.size();i++) textures[i].unbind(i);
}