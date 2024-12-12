#include <rendering/opengl/quad.hpp>

GLFullscreenQuad::GLFullscreenQuad(){
    std::array<float, 20> quad_data = {
        -1.0f, 1.0f,  0.0f, 1.0f, 
         1.0f, 1.0f,  1.0f, 1.0f, 
        -1.0f,-1.0f,  0.0f, 0.0f, 
         1.0f,-1.0f,  1.0f, 0.0f, 
    };

    quad_buffer.initialize(quad_data.size());
    quad_buffer.insert(0, quad_data.size(), quad_data.data());

    vao.attachBuffer(&quad_buffer, {VEC2, VEC2});
}

void GLFullscreenQuad::render(){
    vao.bind();
    quad_buffer.bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    vao.unbind();
}