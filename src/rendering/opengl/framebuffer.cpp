#include <rendering/opengl/framebuffer.hpp>

static uint currently_bound = 0;
GLFramebuffer::GLFramebuffer(int width, int height, std::vector<FramebufferTexture> texture_definitions): width(width), height(height){
    GL_CALL( glGenFramebuffers(1, &framebuffer_id));
    bind();

    GL_CALL( glGenRenderbuffers(1, &depth_renderbuffer_id));
    GL_CALL( glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer_id));

    GL_CALL( glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height));
    GL_CALL( glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer_id));

    GL_CALL( glBindRenderbuffer(GL_RENDERBUFFER, 0));

    size_t textures_total = texture_definitions.size();
    textures.resize(textures_total);

    std::vector<uint> attachments(textures_total);
    for(size_t i = 0; i < textures.size();i++){
        auto& definition = texture_definitions[i];
        auto& texture = textures[i];

        texture.configure(definition.internal_format, definition.format, definition.data_type, width, height);

        attachments[i] = GL_COLOR_ATTACHMENT0 + i;
        
        glFramebufferTexture2D(GL_FRAMEBUFFER,  attachments[i] , GL_TEXTURE_2D, texture.getID(), 0);
    }

    GL_CALL( glDrawBuffers(textures_total, attachments.data()));

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) 
        throw std::runtime_error("Failed to create framebuffer!");

    unbind();
}
GLFramebuffer::~GLFramebuffer(){
    glDeleteFramebuffers(1, &framebuffer_id); // Delete framebuffer
    glDeleteRenderbuffers(1, &depth_renderbuffer_id); // Delete depth renderbuffer
}
void GLFramebuffer::bind(){
    if(currently_bound == framebuffer_id) return;
    GL_CALL( glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id));
    currently_bound = framebuffer_id;
}
void GLFramebuffer::unbind(){
    if(currently_bound == 0) return;
    GL_CALL( glBindFramebuffer(GL_FRAMEBUFFER, 0));
    currently_bound = 0;
}


void GLFramebuffer::bindTextures(){
    for(size_t  i = 0;i < textures.size();i++) textures[i].bind(i);
}
void GLFramebuffer::unbindTextures(){
    for(size_t  i = 0;i < textures.size();i++) textures[i].unbind(i);
}