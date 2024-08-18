#include <rendering/buffer.hpp>

void checkGLError(const char *file, int line){
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        const char *errorString;
        switch (error) {
            case GL_INVALID_ENUM:                  errorString = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 errorString = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             errorString = "GL_INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                errorString = "GL_STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               errorString = "GL_STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 errorString = "GL_OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: errorString = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            default:                               errorString = "Unknown error"; break;
        }
        fprintf(stderr, "OpenGL error in file %s at line %d: %s\n", file, line, errorString);
    }
}

GLBuffer newBuffer(){
    GLBuffer buffer;

    glGenBuffers(1, &buffer.data);
    glGenBuffers(1, &buffer.index);
    glGenVertexArrays(1, &buffer.vao);

    return buffer;
}

void destroyBuffer(GLBuffer buffer){
    glDeleteBuffers(1 , &buffer.data);
    glDeleteBuffers(1 , &buffer.index);
    glDeleteVertexArrays(1, &buffer.vao);
}

void loadMeshToBuffer(Mesh* mesh, GLBuffer* glbuffer){
    unsigned int buffer = glbuffer->data;
    unsigned int index_buffer = glbuffer->index;
    unsigned int vao = glbuffer->vao;

    //printf("%u %u %u\n", buffer, index_buffer, vao);
    //printf("Loading data into buffer: %i index buffer: %i\n", buffer, index_buffer);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, mesh->vertices_count * sizeof(float), mesh->vertices, GL_DYNAMIC_DRAW);
    
    CHECK_GL_ERROR();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices_count * sizeof(unsigned int), mesh->indices, GL_DYNAMIC_DRAW);
    
    CHECK_GL_ERROR();

    size_t stride =  mesh->vertex_size * sizeof(float);

    int size_to_now = 0;
    for(int i = 0;i < mesh->format_size;i++){
        size_t current_size = mesh->vertex_format[i];
        
        uintptr_t pointer = size_to_now * sizeof(float);

        //printf("Size: %lu Pointer: %lu Stride: %lu\n",current_size,pointer,stride);

        glVertexAttribPointer(i, current_size, GL_FLOAT, GL_FALSE, stride, (void*)pointer);
        glEnableVertexAttribArray(i);

        CHECK_GL_ERROR();

        size_to_now += current_size;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glbuffer->vertexCount = mesh->vertices_count;
    glbuffer->indiciesCount = mesh->indices_count;
}

void drawBuffer(GLBuffer* glbuffer){
    unsigned int buffer = glbuffer->data;
    unsigned int index_buffer = glbuffer->index;
    unsigned int vao = glbuffer->vao;

    //printf("%u %u %u\n", buffer, index_buffer, vao);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

    // Draw the triangles !
    glDrawElements(
        GL_TRIANGLES,      // mode
        glbuffer->indiciesCount,    // count
        GL_UNSIGNED_INT,   // type
        (void*)0           // element array buffer offset
    );

    CHECK_GL_ERROR();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}