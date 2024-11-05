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

/*GLStripBuffer::GLStripBuffer(VertexFormat format) {
    setFormat(format);

    glGenVertexArrays(1, &this->vao);
    glGenBuffers(1, &this->data);
}
GLStripBuffer::~GLStripBuffer(){
    glDeleteBuffers(1 , &this->data);
    glDeleteVertexArrays(1, &this->vao);
}

void GLStripBuffer::setFormat(VertexFormat format){
    this->format = format;
    applyFormat();
}

void GLStripBuffer::applyFormat(){
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, this->data);

    this->format.apply();
}
// Appends data and returns its starting point
size_t GLStripBuffer::appendData(const float* data, size_t size){
    size_t where = data_size;
    insertData(data_size, data, size);
    return data_size;
}

// Inserts data at a given location
bool GLStripBuffer::insertData(size_t start, const float* data, size_t size){
    if(!data) return false;
    if(start + size >= buffer_size){
        if(!resize(start + size)) return false;

        data_size = buffer_size;
    }

    glBindBuffer(GL_ARRAY_BUFFER, this->data);
    glBufferSubData(GL_ARRAY_BUFFER, start * sizeof(float), size * sizeof(float), data);
    
    return true;
}

void GLStripBuffer::draw(){
    uint32_t buffer = this->data;

    glBindVertexArray(vao);
    glDrawArrays(
        GL_TRIANGLE_STRIP,
        0,
        data_size          
    );
    //glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
*/

void GLDoubleBuffer::swap(){
    this->current = !this->current;
}
GLBuffer& GLDoubleBuffer::getBuffer(){
    return this->buffers[this->current];
}
GLBuffer& GLDoubleBuffer::getBackBuffer(){
    return this->buffers[!this->current];
}
void MultiChunkBuffer::unloadFarawayChunks(const glm::ivec3& from, float treshold){
    for(auto& [position, chunk]: this->loadedChunks){
        float distance = glm::distance(glm::vec3(from), glm::vec3(position));
        if(distance <= treshold) continue;
        unloadChunkMesh(position);
    }
}

/*
    Creates a persistently mapped buffer,
    returns a tuple of [buffer_id, pointer to mapped data]
*/
template <typename T>
static std::tuple<uint32_t, T*> createPersistentBuffer(size_t size, uint32_t type){
    uint32_t buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(type, buffer);
    glBufferStorage(type, size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

    T* pointer = static_cast<T*>(glMapBufferRange(type, 0, size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT));

    if(!pointer) {
        throw std::runtime_error("Failed to map persistent buffer.");
    }

    return {buffer,pointer};
}


std::string formatSize(size_t bytes) {
    const char* suffixes[] = {"B", "KB", "MB", "GB", "TB"};
    size_t suffixIndex = 0;
    double size = static_cast<double>(bytes);

    // Loop to reduce size and find appropriate suffix
    while (size >= 1024 && suffixIndex < 4) {  // Up to TB
        size /= 1024;
        ++suffixIndex;
    }

    // Format size with 2 decimal places
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << size << " " << suffixes[suffixIndex];
    return out.str();
}


void MultiChunkBuffer::initialize(uint32_t renderDistance){
    vertexFormat = VertexFormat({3,1,2,1,1});
    
    maxDrawCalls = pow(renderDistance * 2, 3);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /*
        Create and map buffer for draw calls
    */

    std::tie(indirectBuffer, persistentDrawCallBuffer) = createPersistentBuffer<DrawElementsIndirectCommand>(
        sizeof(DrawElementsIndirectCommand) * maxDrawCalls * 2,
        GL_DRAW_INDIRECT_BUFFER
    );

    drawCallBufferSize = maxDrawCalls;

    CHECK_GL_ERROR();

    /*
        Create and map vertex and index buffers
    */
    size_t totalMemoryToAllocate = (1024ULL * 1024ULL * 1024ULL) / 2ULL; // 500MB of video memory

    size_t vertexPerFace = vertexFormat.getVertexSize() * 4; // For vertices per face
    size_t indexPerFace  = 6;

    size_t total = vertexPerFace + indexPerFace;
    size_t segment = totalMemoryToAllocate / total;

    size_t vertexSpaceTotal = segment * vertexPerFace;
    size_t indexSpaceTotal = segment * indexPerFace;
    
    size_t maxVertexCount = vertexSpaceTotal / sizeof(float);
    size_t maxIndexCount  = indexSpaceTotal / sizeof(uint32_t);

    std::cout << "Video RAM allocated for vertices total: " << formatSize(vertexSpaceTotal) << std::endl;
    std::cout << "Video RAM allocated for indices total : " << formatSize(indexSpaceTotal)  << std::endl;
    std::cout << "Total Video RAM allocated: " << formatSize(vertexSpaceTotal + indexSpaceTotal) << std::endl;

    vertexAllocator = Allocator(maxVertexCount, [this](size_t requested_amount) {return false;});
    indexAllocator = Allocator(maxIndexCount, [this](size_t requested_amount) {return false;});

    std::tie(vertexBuffer, persistentVertexBuffer) = createPersistentBuffer<float>(
        maxVertexCount * sizeof(float),
        GL_ARRAY_BUFFER
    );

    CHECK_GL_ERROR();

    std::tie(indexBuffer, persistentIndexBuffer) = createPersistentBuffer<uint32_t>(
        maxIndexCount * sizeof(uint32_t),
        GL_ELEMENT_ARRAY_BUFFER
    );

    CHECK_GL_ERROR();

    vertexFormat.apply();
}

MultiChunkBuffer::~MultiChunkBuffer(){
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &indexBuffer);
    glDeleteBuffers(1,  &indirectBuffer);
    glDeleteVertexArrays(1, &vao);
}
std::tuple<bool,size_t,size_t> MultiChunkBuffer::allocateAndUploadMesh(Mesh& mesh){
    auto vertexAlloc = vertexAllocator.allocate(mesh.getVertices().size());
    auto indexAlloc = indexAllocator.allocate(mesh.getIndices().size());

    if(vertexAlloc.failed || indexAlloc.failed) return {false,0,0};

    size_t vertexBufferOffset = vertexAlloc.location;
    size_t indexBufferOffset = indexAlloc.location;

    /*
        Allocate space for vertex data and save it
    */
    std::memcpy(
        persistentVertexBuffer + vertexBufferOffset, // Copy to the vertex buffer at the offset
        mesh.getVertices().data(), // Copy the vertex data
        mesh.getVertices().size() *  sizeof(float) // Copy the size of the data in bytes
    );
    //mappedVertexBuffer + vertexBufferOffset, mesh.getVertices().data(), mesh.getVertices().size() *  sizeof(GLfloat)
    
    std::memcpy(
        persistentIndexBuffer + indexBufferOffset, // Copy to the vertex buffer at the offset
        mesh.getIndices().data(), // Copy the vertex data
        mesh.getIndices().size() *  sizeof(uint32_t) // Copy the size of the data in bytes
    );

    return {true, vertexBufferOffset, indexBufferOffset};
}

bool MultiChunkBuffer::addChunkMesh(Mesh& mesh, const glm::ivec3& pos){
    if(loadedChunks.count(pos) != 0) return false;
    if(mesh.getVertices().size() == 0) return false;
    
    //auto start  = std::chrono::high_resolution_clock::now();

    auto [success, vertexBufferOffset, indexBufferOffset] = allocateAndUploadMesh(mesh);
    if(!success) return false;
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
    
    return true;
}
bool MultiChunkBuffer::swapChunkMesh(Mesh& mesh, const glm::ivec3& pos){
    if(loadedChunks.count(pos) == 0) return false;
    
    auto [success, vertexBufferOffset, indexBufferOffset] = allocateAndUploadMesh(mesh);
    if(!success) return false;

    LoadedChunk& chunk = loadedChunks.at(pos);

    vertexAllocator.free(chunk.vertexData);
    indexAllocator .free(chunk.indexData);
    /*
        Change the draw call arguments
    */
    chunk.vertexData = vertexBufferOffset;
    chunk.indexData = indexBufferOffset;
    
    chunk.firstIndex = indexBufferOffset;
    chunk.count = mesh.getIndices().size();
    chunk.baseVertex = vertexBufferOffset / vertexFormat.getVertexSize();

    return true;
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
    std::memcpy(persistentDrawCallBuffer + bufferOffset, commands.data(), sizeof(DrawElementsIndirectCommand) * commands.size());

    drawCallCount = commands.size();

    std::memcpy(persistentDrawCallBuffer, commands.data(), sizeof(DrawElementsIndirectCommand) * commands.size());

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