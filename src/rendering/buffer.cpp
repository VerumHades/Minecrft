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
    uint32_t buffer = this->data;
    uint32_t index_buffer = this->index;
    
    glBindVertexArray(vao);

    CHECK_GL_ERROR();;

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, mesh.getVertices().size() * sizeof(float), mesh.getVertices().data(), GL_DYNAMIC_DRAW);
    
    CHECK_GL_ERROR();;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.getIndices().size() * sizeof(uint32_t),mesh.getIndices().data(), GL_DYNAMIC_DRAW);
    
    CHECK_GL_ERROR();;

    size_t stride =  mesh.vertexSize * sizeof(float);

    size_t size_to_now = 0;
    for(int i = 0;i < mesh.getFormat().size();i++){
        size_t current_size = mesh.getFormat()[i];
        
        uintptr_t pointer = size_to_now * sizeof(float);

        //printf("Size: %lu Pointer: %lu Stride: %lu\n",current_size,pointer,stride);

        glVertexAttribPointer(i, (int) current_size, GL_FLOAT, GL_FALSE, (int)stride, (void*)pointer);
        glEnableVertexAttribArray(i);

        CHECK_GL_ERROR();;

        size_to_now += current_size;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    this->vertexCount   = mesh.getVertices().size();
    this->indiciesCount = mesh.getIndices().size();

    this->dataLoaded = true;
}

void GLBuffer::draw(){
    uint32_t buffer = this->data;
    uint32_t index_buffer =this->index;

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
    uint32_t buffer = this->data;
    uint32_t index_buffer =this->index;

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

MultiChunkBuffer::MultiChunkBuffer(uint32_t maxDrawCalls): maxDrawCalls(maxDrawCalls){
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /*
        These values are gross estimates and will probably need dynamic adjusting later
    */
    maxVertices = maxDrawCalls * 500000; // Estimate that every chunk has about 500000 vertices at max
    maxIndices = maxDrawCalls * 80000; // Same for indices

    /*
        Create and map buffer for draw calls
    */
    glGenBuffers(1, &indirectBuffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand) * maxDrawCalls, nullptr, GL_STATIC_DRAW);
    drawCallBuffer = static_cast<DrawElementsIndirectCommand*>(glMapBuffer(GL_DRAW_INDIRECT_BUFFER, GL_WRITE_ONLY));

    CHECK_GL_ERROR();

    for(int i = 0;i < maxDrawCalls;i++) {freeDrawCallIndices.push(i);}

    /*
        Create and map vertex and index buffers
    */
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, maxVertices * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    vertexAllocator = std::make_unique<Allocator<GLfloat>>(static_cast<GLfloat*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)), maxVertices * sizeof(GLfloat));
    
    CHECK_GL_ERROR();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIndices * sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);
    indexAllocator = std::make_unique<Allocator<GLuint>>(static_cast<GLuint*>(glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY)), maxIndices * sizeof(GLuint));

    CHECK_GL_ERROR();
}

MultiChunkBuffer::~MultiChunkBuffer(){
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
    glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexBuffer);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}

MultiChunkBuffer::DrawCall MultiChunkBuffer::addDrawCall(uint32_t firstIndex, uint32_t count){
    if(freeDrawCallIndices.empty()){
        std::cout << "MultiChunkBuffer cannot create any more draw calls, it completely full!" << std::endl;
        return {0,0,0};
    }

    uint32_t index = freeDrawCallIndices.front();
    freeDrawCallIndices.pop();

    drawCallBuffer[index].count = count;
    drawCallBuffer[index].instanceCount = 0;
    drawCallBuffer[index].firstIndex = firstIndex;
    drawCallBuffer[index].baseVertex = 0;
    drawCallBuffer[index].baseInstance = 0;

    return {firstIndex, count, index};
}

void MultiChunkBuffer::addChunkMesh(Mesh& mesh, const glm::vec3& pos){
    if(loadedChunks.count(pos) != 0) return;

    /*
        Allocate space for vertex data and save it
    */
    size_t vertexDataSize = mesh.getVertices().size() * sizeof(GLfloat);
    GLfloat* vertexData = vertexAllocator->allocate(vertexDataSize);
    memcpy(vertexData, mesh.getVertices().data(), vertexDataSize);

    /*
        Allocate data for index data, find position of vertex data and offset the indices
    */
    size_t indexDataSize = mesh.getIndices().size() * sizeof(GLuint);
    GLuint* indexData = indexAllocator->allocate(indexDataSize);
    
    size_t vertexDataOffset = vertexAllocator->getOffset(vertexData);
    for(size_t i = 0;i < mesh.getIndices().size();i++){
        indexData[i] = vertexDataOffset + mesh.getIndices()[i];
    }

    /*
        Register the chunks draw call and save it as loaded
    */
    size_t indexDataOffset = indexAllocator->getOffset(indexData);
    DrawCall call = addDrawCall(indexDataOffset, mesh.getIndices().size());

    loadedChunks[pos] = {
        vertexData,
        indexData,
        call
    };
}
void MultiChunkBuffer::draw(){
    glBindVertexArray(vao);

    int drawCalls = maxDrawCalls - freeDrawCallIndices.size();
    
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, drawCalls, sizeof(DrawElementsIndirectCommand));
}