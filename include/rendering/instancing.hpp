#pragma once

#include <rendering/opengl/buffer.hpp>

class GLInstancedBuffer: public GLBuffer<float,GL_ARRAY_BUFFER>{
    private:
        GLBuffer<float, GL_ARRAY_BUFFER> vertex_buffer;
        GLBuffer<uint , GL_ELEMENT_ARRAY_BUFFER> index_buffer;

        
    public:

};