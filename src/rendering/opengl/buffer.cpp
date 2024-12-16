#include <rendering/opengl/buffer.hpp>

void checkGLError(const char *file, int line){
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        const char *errorString;
        switch (error) {
            case GL_INVALID_ENUM:                  errorString = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 errorString = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             errorString = "GL_INVALID_OPERATION"; break;
            //case GL_STACK_OVERFLOW:                errorString = "GL_STACK_OVERFLOW"; break;
            //case GL_STACK_UNDERFLOW:               errorString = "GL_STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 errorString = "GL_OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: errorString = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            default:                               errorString = "Unknown error"; break;
        }
        std::cerr << "OpenGL error in file " << file << " at line " << line << " " << errorString << std::endl;
        //throw std::runtime_error("Opengl error.");
    }
}

GLVertexFormat::GLVertexFormat(std::initializer_list<GLVertexValueType> bindings, bool per_instance = false): per_instance(per_instance), bindings(bindings){
    totalSize = 0;
    for(auto& size: bindings) totalSize += size;
}   

void GLVertexFormat::apply(uint& slot){
    size_t stride =  totalSize * sizeof(float);
    size_t size_to_now = 0;

    for(auto& current_size: bindings){
        uintptr_t pointer = size_to_now * sizeof(float);

        glVertexAttribPointer(slot, (int) current_size, GL_FLOAT, GL_FALSE, (int)stride, (void*)pointer);
        glEnableVertexAttribArray(slot);
        if(per_instance) glVertexAttribDivisor(slot, 1);

        size_to_now += current_size;
        slot++;
    }
}