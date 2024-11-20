#include <rendering/buffer.hpp>

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

GLBuffer::GLBuffer() {
    glGenVertexArrays(1, &this->vao);
    glGenBuffers(1, &this->data);
    glGenBuffers(1, &this->index);

    //std::cerr << "GLBuffer constructor called for object at " << this << std::endl;
}
GLBuffer::~GLBuffer(){
    glDeleteBuffers(1 , &this->data);
    glDeleteBuffers(1 , &this->index);
    glDeleteVertexArrays(1, &this->vao);

    //std::cerr << "GLBuffer destructor called for object at " << this << std::endl;
}

void GLBuffer::loadMesh(Mesh& mesh){
    uint buffer = this->data;
    uint index_buffer = this->index;
    
    glBindVertexArray(vao);

    CHECK_GL_ERROR();;

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, mesh.getVertices().size() * sizeof(float), mesh.getVertices().data(), GL_DYNAMIC_DRAW);
    
    CHECK_GL_ERROR();;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.getIndices().size() * sizeof(uint),mesh.getIndices().data(), GL_DYNAMIC_DRAW);
    
    CHECK_GL_ERROR();;

    mesh.getFormat().apply();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    this->vertexCount   = mesh.getVertices().size();
    this->indiciesCount = mesh.getIndices().size();

    this->dataLoaded = true;
}

void GLBuffer::draw(){
    uint buffer = this->data;
    uint index_buffer =this->index;

    //printf("%u %u %u\n", buffer, index_buffer, vao);

    glBindVertexArray(vao);

    //glBindBuffer(GL_ARRAY_BUFFER, buffer);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer)

    glDrawElements(
        GL_TRIANGLES,      // mode
        (int) this->indiciesCount,    // count
        GL_UNSIGNED_INT,   // type
        (void*)0           // element array buffer offset
    );

    CHECK_GL_ERROR();;

    //glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void GLBuffer::drawInstances(int count){
    uint buffer = this->data;
    uint index_buffer =this->index;

    //printf("%u %u %u\n", buffer, index_buffer, vao);

    glBindVertexArray(vao);

   // glBindBuffer(GL_ARRAY_BUFFER, buffer);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

    CHECK_GL_ERROR();;

    //std::cout << this->indiciesCount << "Instances:" << count << std::endl;

    glDrawElementsInstanced(
        GL_TRIANGLES,      // mode
        (int) this->indiciesCount,    // count
        GL_UNSIGNED_INT,   // type
        (void*)0,           // element array buffer offset,
        count
    );

    CHECK_GL_ERROR();;

    //glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void GLDoubleBuffer::swap(){
    this->current = !this->current;
}
GLBuffer& GLDoubleBuffer::getBuffer(){
    return this->buffers[this->current];
}
GLBuffer& GLDoubleBuffer::getBackBuffer(){
    return this->buffers[!this->current];
}
