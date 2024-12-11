#include <rendering/gbuffer.hpp>

GBuffer::GBuffer(int width, int height): width(width), height(height), GLFramebuffer(){
    resize(width, height);

    bind();

    glGenRenderbuffers(1, &depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    unbind();
    
    uint attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    activateAttachments(attachments, 3);



    std::array<float, 20> quad_data = {
        -1.0f, 1.0f,  0.0f, 1.0f, 
         1.0f, 1.0f,  1.0f, 1.0f, 
        -1.0f,-1.0f,  0.0f, 0.0f, 
         1.0f,-1.0f,  1.0f, 0.0f, 
    };

    quad_buffer.initialize(quad_data.size());
    quad_buffer.insert(0, quad_data.size(), quad_data.data());

    vao.attachBuffer(&quad_buffer, {VEC2, VEC2});

    shader_program.setSamplerSlot("gPosition", 0);
    shader_program.setSamplerSlot("gNormal", 1);
    shader_program.setSamplerSlot("gAlbedoSpec", 2);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) 
        throw std::runtime_error("Failed to create gbuffer!");
}

void GBuffer::resize(int width, int height){
    positionTexture.configure(GL_RGBA16F, GL_FLOAT, width, height);
    normalTexture.configure(GL_RGBA16F, GL_FLOAT, width, height);
    albedoTexture.configure(GL_RGBA, GL_UNSIGNED_BYTE, width, height);

    attach(0, positionTexture);
    attach(1, normalTexture);
    attach(2, albedoTexture);
}

void GBuffer::render(){
    unbind();
    shader_program.use();
    vao.bind();

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    positionTexture.bind(0);
    normalTexture.bind(1);
    albedoTexture.bind(2);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    positionTexture.unbind(0);
    normalTexture.unbind(1);
    albedoTexture.unbind(2);

    vao.unbind();
}