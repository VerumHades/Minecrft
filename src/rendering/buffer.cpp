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
void setupFormat(std::vector<int> format){
    int vertexSize = 0;
    for(auto& sz: format) vertexSize += sz;

    size_t stride =  vertexSize * sizeof(float);

    size_t size_to_now = 0;
    for(int i = 0;i < format.size();i++){
        size_t current_size = format[i];
        
        uintptr_t pointer = size_to_now * sizeof(float);

        //printf("Size: %lu Pointer: %lu Stride: %lu\n",current_size,pointer,stride);

        glVertexAttribPointer(i, (int) current_size, GL_FLOAT, GL_FALSE, (int)stride, (void*)pointer);
        glEnableVertexAttribArray(i);

        CHECK_GL_ERROR();;

        size_to_now += current_size;
    }
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

    setupFormat(mesh.getFormat());

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

void MultiChunkBuffer::initialize(uint32_t maxDrawCalls_){
    maxDrawCalls = maxDrawCalls_;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    CHECK_GL_ERROR();
    /*
        These values are gross estimates and will probably need dynamic adjusting later
    */
    maxVertices = maxDrawCalls * 5000; // Estimate that every chunk has about 50000 vertices at max
    maxIndices = maxDrawCalls * 500; // Same for indices
    vertexAllocator = Allocator(maxVertices);
    indexAllocator = Allocator(maxIndices);

    /*
        Create and map buffer for draw calls
    */
    glGenBuffers(1, &indirectBuffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand) * maxDrawCalls, nullptr, GL_STATIC_DRAW);

    CHECK_GL_ERROR();

    /*
        Create and map vertex and index buffers
    */
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, maxVertices * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    CHECK_GL_ERROR();

    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIndices * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

    CHECK_GL_ERROR();

    setupFormat({3,3,2,1,1});
}

MultiChunkBuffer::~MultiChunkBuffer(){
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
    glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexBuffer);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}

/*void MultiChunkBuffer::setDrawCall(size_t index, uint32_t firstIndex, uint32_t count, uint32_t baseVertex){
    if(freeDrawCallIndices.empty()){
        std::cout << "MultiChunkBuffer cannot create any more draw calls, it completely full!" << std::endl;
        return;
    }

    drawCallBuffer = static_cast<DrawElementsIndirectCommand*>(glMapBuffer(GL_DRAW_INDIRECT_BUFFER, GL_WRITE_ONLY));

    drawCallBuffer[index].count = count;
    drawCallBuffer[index].instanceCount = 1;
    drawCallBuffer[index].firstIndex = firstIndex;
    drawCallBuffer[index].baseVertex = baseVertex;
    drawCallBuffer[index].baseInstance = 0;

    glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
}*/

void MultiChunkBuffer::addChunkMesh(Mesh& mesh, const glm::vec3& pos){
    if(loadedChunks.count(pos) != 0) return;
    if(mesh.getVertices().size() == 0) return;

    /*
        Allocate space for vertex data and save it
    */
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    size_t vertexBufferOffset = vertexAllocator.allocate(mesh.getVertices().size());
    glBufferSubData(GL_ARRAY_BUFFER, vertexBufferOffset * sizeof(float), mesh.getVertices().size() *  sizeof(float), mesh.getVertices().data());
    //mappedVertexBuffer + vertexBufferOffset, mesh.getVertices().data(), mesh.getVertices().size() *  sizeof(GLfloat)
    /*
        Allocate data for index data, find position of vertex data and offset the indices
    */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    size_t indexBufferOffset = indexAllocator.allocate(mesh.getIndices().size());
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indexBufferOffset * sizeof(uint32_t), mesh.getIndices().size() * sizeof(uint32_t), mesh.getIndices().data());

    /*
        Register the chunks draw call and save it as loaded
    */
    //DrawCall call = addDrawCall(indexBufferOffset, mesh.getIndices().size(), vertexBufferOffset / vertexSize);

    loadedChunks[pos] = {
        vertexBufferOffset,
        indexBufferOffset,

        indexBufferOffset,
        mesh.getIndices().size(),
        vertexBufferOffset / vertexSize
    };
}

void MultiChunkBuffer::addDrawCall(const glm::vec3& position){
    if(loadedChunks.count(position) == 0) return;

    LoadedChunk& chunk = loadedChunks.at(position);
    chunk.hasDrawCall = true;

    DrawElementsIndirectCommand command = {chunk.count,1,chunk.firstIndex,chunk.baseVertex,0};
    drawCalls.push_back(command);

    //setDrawCall(index, chunk.firstIndex, chunk.count, chunk.baseVertex);
}
void MultiChunkBuffer::removeDrawCall(const glm::vec3& position){
    if(loadedChunks.count(position) == 0) return;

    LoadedChunk& chunk = loadedChunks.at(position);
    if(!chunk.hasDrawCall) return;

    //DrawElementsIndirectCommand command = {chunk.count,1,chunk.firstIndex,chunk.baseVertex,0};
   // drawCalls.erase(command);
}

void MultiChunkBuffer::updateDrawCalls(){
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand) * drawCalls.size(), drawCalls.data(), GL_STATIC_DRAW);
}

void MultiChunkBuffer::draw(){
    glBindVertexArray(vao);

    //int drawCalls = maxDrawCalls - freeDrawCallIndices.size();
    //std::cout << "Active draw calls: " << lastDrawCall << std::endl;
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, drawCalls.size(), sizeof(DrawElementsIndirectCommand));

    CHECK_GL_ERROR();
}