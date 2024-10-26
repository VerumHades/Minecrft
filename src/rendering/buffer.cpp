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

    mesh.getFormat().apply();

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

static void resizeBuffer(uint32_t* buffer, size_t currentSize, size_t newSize){
    GLuint newBuffer;
    glGenBuffers(1, &newBuffer);
    glBindBuffer(GL_COPY_WRITE_BUFFER, newBuffer);
    glBufferData(GL_COPY_WRITE_BUFFER, newSize, nullptr, GL_STATIC_DRAW);
    
    glBindBuffer(GL_COPY_READ_BUFFER, *buffer);
    glBindBuffer(GL_COPY_WRITE_BUFFER, newBuffer); 

    GLsizeiptr copySize = std::min(currentSize, newSize); 
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, copySize);

    glDeleteBuffers(1, buffer);

    *buffer = newBuffer;
}
void MultiChunkBuffer::unloadFarawayChunks(const glm::ivec3& from, float treshold){
    for(auto& [position, chunk]: this->loadedChunks){
        float distance = glm::distance(glm::vec3(from), glm::vec3(position));
        if(distance <= treshold) continue;
        unloadChunkMesh(position);
    }
}

void MultiChunkBuffer::initialize(uint32_t renderDistance){
    vertexFormat = VertexFormat({3,1,2,1,1});

    maxDrawCalls = pow(renderDistance*2, 3);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    CHECK_GL_ERROR();
    /*
        These values are gross estimates and will probably need dynamic adjusting later
    */
    maxVertices = maxDrawCalls * vertexFormat.getVertexSize() * 10; // Estimate that every chunk has about 50000 vertices at max
    maxIndices = maxDrawCalls * 120; // Same for indices
    vertexAllocator = Allocator(maxVertices, [this](size_t requested){
        // Atempt to trash unused chunks
        return false;
        
    });
    indexAllocator = Allocator(maxIndices, [this](size_t requested){
        return false;
    });

    /*
        Create and map buffer for draw calls
    */
    GLsizeiptr bufferSize = sizeof(DrawElementsIndirectCommand) * maxDrawCalls * 2;

    glGenBuffers(1, &indirectBuffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);

    glBufferStorage(GL_DRAW_INDIRECT_BUFFER, bufferSize, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

    persistentMappedBuffer = static_cast<DrawElementsIndirectCommand*>(
        glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, bufferSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT)
    );

    if (!persistentMappedBuffer) {
        throw std::runtime_error("Failed to map persistent buffer for multichunk buffer.");
    }

    drawCallBufferSize = maxDrawCalls;

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

    vertexFormat.apply();
}

MultiChunkBuffer::~MultiChunkBuffer(){
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &indexBuffer);
    glDeleteBuffers(1,  &indirectBuffer);
    glDeleteVertexArrays(1, &vao);
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

void MultiChunkBuffer::addChunkMesh(Mesh& mesh, const glm::ivec3& pos){
    if(loadedChunks.count(pos) != 0) return;
    if(mesh.getVertices().size() == 0) return;
    
    //auto start  = std::chrono::high_resolution_clock::now();

    auto vertexAlloc = vertexAllocator.allocate(mesh.getVertices().size());
    auto indexAlloc = indexAllocator.allocate(mesh.getIndices().size());
    if(vertexAlloc.failed || indexAlloc.failed) return;

    size_t vertexBufferOffset = vertexAlloc.location;
    size_t indexBufferOffset = indexAlloc.location;

    /*
        Allocate space for vertex data and save it
    */
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, vertexBufferOffset * sizeof(float), mesh.getVertices().size() *  sizeof(float), mesh.getVertices().data());
    //mappedVertexBuffer + vertexBufferOffset, mesh.getVertices().data(), mesh.getVertices().size() *  sizeof(GLfloat)
    
    CHECK_GL_ERROR();
    /*
        Allocate data for index data, find position of vertex data and offset the indices
    */
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indexBufferOffset * sizeof(uint32_t), mesh.getIndices().size() * sizeof(uint32_t), mesh.getIndices().data());

    CHECK_GL_ERROR();
    /*
        Register the chunk save it as loaded
    */
    loadedChunks[pos] = {
        vertexBufferOffset,
        indexBufferOffset,

        indexBufferOffset,
        mesh.getIndices().size(),
        vertexBufferOffset / vertexFormat.getVertexSize(),

        mesh.getVertices().size(),
        mesh.getIndices().size()
    };

    //auto end = std::chrono::high_resolution_clock::now();

    //auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    //std::cout << "Mesh allocated and updated in: " << duration << " microseconds" << std::endl;
}
void MultiChunkBuffer::swapChunkMesh(Mesh& mesh, const glm::ivec3& pos){
    if(loadedChunks.count(pos) == 0) return;
    
    auto vertexAlloc = vertexAllocator.allocate(mesh.getVertices().size());
    auto indexAlloc = indexAllocator.allocate(mesh.getIndices().size());
    if(vertexAlloc.failed || indexAlloc.failed) return;
    
    size_t vertexBufferOffset = vertexAlloc.location;
    size_t indexBufferOffset = indexAlloc.location;

    LoadedChunk& chunk = loadedChunks.at(pos);
    /*
        Allocate space for new vertex data and save it
    */
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, vertexBufferOffset * sizeof(float), mesh.getVertices().size() *  sizeof(float), mesh.getVertices().data());
    CHECK_GL_ERROR();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indexBufferOffset * sizeof(uint32_t), mesh.getIndices().size() * sizeof(uint32_t), mesh.getIndices().data());
    CHECK_GL_ERROR();
    /*
        Change the draw call arguments
    */
    size_t oldVertexBufferOffset = chunk.vertexData;
    size_t oldIndexBufferOffset = chunk.indexData;

    //bool drawCall = drawCallMap.count(pos);
    //if(drawCall) removeDrawCall(pos);

    chunk.vertexData = vertexBufferOffset;
    chunk.indexData = indexBufferOffset;
    
    chunk.firstIndex = indexBufferOffset;
    chunk.count = mesh.getIndices().size();
    chunk.baseVertex = vertexBufferOffset / vertexFormat.getVertexSize();

    //if(drawCall) addDrawCall(pos);
    /*
        Free the old data
    */

    vertexAllocator.free(oldVertexBufferOffset);
    indexAllocator.free(oldIndexBufferOffset);

    //updateDrawCalls();
}
void MultiChunkBuffer::unloadChunkMesh(const glm::ivec3& pos){
    if(loadedChunks.count(pos) == 0) return;
    //removeDrawCall(pos);
    vertexAllocator.free(loadedChunks[pos].vertexData);
    indexAllocator.free(loadedChunks[pos].indexData);
    loadedChunks.erase(pos);
}

DrawElementsIndirectCommand MultiChunkBuffer::getCommandFor(const glm::ivec3& position){
    if(loadedChunks.count(position) == 0) return {};

    LoadedChunk& chunk = loadedChunks.at(position);

    return {static_cast<GLuint>(chunk.count),1,static_cast<GLuint>(chunk.firstIndex),static_cast<GLuint>(chunk.baseVertex),0};
}

void MultiChunkBuffer::updateDrawCalls(std::vector<DrawElementsIndirectCommand>& commands){
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);

    if (commands.size() > maxDrawCalls) {
        throw std::runtime_error("Exceeded maximum number of draw calls.");
    }

    bufferOffset = (bufferOffset + maxDrawCalls) % (maxDrawCalls * 2);
    std::memcpy(persistentMappedBuffer + bufferOffset, commands.data(), sizeof(DrawElementsIndirectCommand) * commands.size());

    drawCallCount = commands.size();

    std::memcpy(persistentMappedBuffer, commands.data(), sizeof(DrawElementsIndirectCommand) * commands.size());

    drawCallCount = commands.size();

    CHECK_GL_ERROR();
}

void MultiChunkBuffer::draw(){
    glBindVertexArray(vao);

    //int drawCalls = maxDrawCalls - freeDrawCallIndices.size();
    //std::cout << "Active draw calls: " << lastDrawCall << std::endl;
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(bufferOffset * sizeof(DrawElementsIndirectCommand)), drawCallCount, sizeof(DrawElementsIndirectCommand));

    CHECK_GL_ERROR();
}

void MultiChunkBuffer::clear(){
    vertexAllocator.clear();
    indexAllocator.clear();
    loadedChunks.clear();
}