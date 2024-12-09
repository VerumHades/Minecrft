#include <rendering/opengl/framebuffer.hpp>

GLFramebuffer::GLFramebuffer(){
    glGenFramebuffers(1, &framebuffer_id);
}
void GLFramebuffer::bind(){
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
}
void GLFramebuffer::unbind(){
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void GLFramebuffer::attach(int attachment, GLTexture2D& texture){
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D, texture.getID(), 0);
}