#include <rendering/opengl/framebuffer.hpp>

static uint currently_bound = 0;
GLFramebuffer::GLFramebuffer(){
    CHECK_GL_ERROR();
    
    glGenFramebuffers(1, &framebuffer_id);

    CHECK_GL_ERROR();
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
void GLFramebuffer::attach(int attachment, GLTexture2D& texture){
    bind();

    uint attachment_number = GL_COLOR_ATTACHMENT0 + attachment;
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_number, GL_TEXTURE_2D, texture.getID(), 0);

    GLenum drawBuffers[] = { attachment_number };
    glDrawBuffers(1, drawBuffers);

    unbind();
}