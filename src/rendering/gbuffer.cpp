#include <rendering/gbuffer.hpp>

GBuffer::GBuffer(int width, int height): 
    GLFramebuffer(width,height, {
        {GL_RGBA16F, GL_FLOAT}, // Positions
        {GL_RGBA16F, GL_FLOAT}, // Normals
        {GL_RGBA,GL_UNSIGNED_BYTE} // Albedo
    }
    ){

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
}

void GBuffer::resize(int width, int height){
    
}

void GBuffer::render(){
    unbind();
    shader_program.updateUniforms();
    vao.bind();
    quad_buffer.bind();
    
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bindTextures();

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    unbindTextures();

    vao.unbind();
}