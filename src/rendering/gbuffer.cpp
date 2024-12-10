#include <rendering/gbuffer.hpp>

GBuffer::GBuffer(int width, int height): width(width), height(height), GLFramebuffer(){
    resize(width, height);

    std::array<float, 20> quad_data = {
        -1.0f, 1.0f,  0.0f, 1.0f, 
         1.0f, 1.0f,  1.0f, 1.0f, 
        -1.0f,-1.0f,  0.0f, 0.0f, 
         1.0f,-1.0f,  1.0f, 0.0f, 
    };

    quad_buffer.initialize(quad_data.size());
    quad_buffer.insert(0, quad_data.size(), quad_data.data());

    vao.attachBuffer(&quad_buffer, {VEC2, VEC2});

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

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    positionTexture.bind(0);
    normalTexture.bind(1);
    albedoTexture.bind(2);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    vao.unbind();
}