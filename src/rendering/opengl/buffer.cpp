#include <rendering/opengl/buffer.hpp>

GLVertexFormat::GLVertexFormat(std::initializer_list<GLVertexValueType> bindings, bool per_instance): per_instance(per_instance), bindings(bindings){
    totalSize = 0;
    for(auto& size: bindings) totalSize += size;
}   

void GLVertexFormat::apply(uint& slot){
    size_t stride =  totalSize * sizeof(float);
    size_t size_to_now = 0;

    for(auto& current_size: bindings){
        uintptr_t pointer = size_to_now * sizeof(float);

        GL_CALL( glVertexAttribPointer(slot, (int) current_size, GL_FLOAT, GL_FALSE, (int)stride, (void*)pointer));
        GL_CALL( glEnableVertexAttribArray(slot));
        if(per_instance) GL_CALL( glVertexAttribDivisor(slot, 1));

        size_to_now += current_size;
        slot++;
    }
}

void GLDrawCallBuffer::clear(){
    draw_commands.clear();
}
void GLDrawCallBuffer::push(GLDrawCallBuffer::DrawCommand& command){
    draw_commands.push_back(command);
}
void GLDrawCallBuffer::push(GLDrawCallBuffer::DrawCommand* commands, size_t size){
    draw_commands.insert(draw_commands.end(), commands, commands + size);
}
void GLDrawCallBuffer::flush(){
    if(draw_commands.size() == 0) return;
    
    if(draw_commands.size() > buffer.size()) buffer.initialize(draw_commands.size(), draw_commands.data());
    else buffer.insert(0, draw_commands.size(), draw_commands.data());
}

void GLDrawCallBuffer::bind(){
    buffer.bind();
}


GLVertexArray::GLVertexArray(){
    GL_CALL( glGenVertexArrays(1,  &vao_id));
}
GLVertexArray::~GLVertexArray(){
    GL_CALL( glDeleteVertexArrays(1,  &vao_id));
}

size_t GLVertexArray::attachBuffer(GLBuffer<float, GL_ARRAY_BUFFER>* buffer_pointer, GLVertexFormat format, int index){
    if(index == -1) index = index_counter++;
    buffers[index] = {buffer_pointer, format};

    update();
    
    return format.getVertexSize();
}

void GLVertexArray::attachBuffer(GLBuffer<uint, GL_ELEMENT_ARRAY_BUFFER>* buffer){
    bind();
    buffer->bind();
    unbind();
}

/*
    Updates buffers, bindings locations are based on how the buffers were attached sequentialy
*/
void GLVertexArray::update(){
    bind();

    uint slot = 0;

    for(auto& [index,element]: buffers){
        element.buffer_pointer->bind();
        element.format.apply(slot);
    }

    unbind();
}
void GLVertexArray::bind() const {
    GL_CALL( glBindVertexArray(vao_id));
}
void GLVertexArray::unbind() const {
    GL_CALL( glBindVertexArray(0));
}

