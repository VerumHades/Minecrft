#include <ui/opengl_backend.hpp>

UIOpenglBackend::UIOpenglBackend(){
    shader_program.use();
    fontManager.initialize();

    vao.bind();

    vao.attachBuffer(&vertex_buffer, {VEC2,VEC4,VEC2,FLOAT});
    vertex_buffer.initialize(1);
    vao.attachIndexBuffer(&index_buffer);
    index_buffer.initialize(1);

    vao.unbind();

    shader_program.setSamplerSlot("textAtlas",1);
}

void UIOpenglBackend::setupRender(){
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    //glClear(GL_DEPTH_BUFFER_BIT);

    shader_program.updateUniforms();

    vao.bind();
    mainFont.getAtlas()->bind(1);
}

void UIOpenglBackend::cleanupRender(){
    glDisable(GL_SCISSOR_TEST);
}

std::list<UIBackend::Batch>::iterator UIOpenglBackend::addRenderBatch(UIRenderBatch& batch){
    size_t vertex_count = batch.commands.size() * 4 * vertex_size;
    size_t index_count = batch.commands.size() * 6;

    size_t vertex_start = vertices.insert(nullptr, vertex_count);
    size_t index_start = indices.insert(nullptr, index_count);

    float* vertex_ptr = vertices.data() + vertex_start;
    uint* index_ptr = indices.data() + index_start;

    int index_offset = vertex_start / vertex_size;
    for(auto& command: batch.commands){
        proccessRenderCommand(command, vertex_ptr, index_ptr, index_offset);
    }

    needs_update = true;

    return batches.insert(batches.end(), {
        batch.clip_min,
        batch.clip_max,
        batch.texture,
        vertex_start,
        index_start,
        vertex_count,
        index_count
    });
}

void UIOpenglBackend::removeBatch(std::list<UIBackend::Batch>::iterator batch_iter){
    vertices.free(batch_iter->vertex_start);
    indices.free(batch_iter->index_start);
    batches.erase(batch_iter);
}

void UIOpenglBackend::renderBatch(std::list<UIBackend::Batch>::iterator batch_iter){
    glScissor(batch_iter->clip_min.x, batch_iter->clip_min.y, batch_iter->clip_max.x - batch_iter->clip_min.x, batch_iter->clip_max.y - batch_iter->clip_min.y);
    glDrawElements(GL_TRIANGLES, batch_iter->index_size, GL_UNSIGNED_INT, reinterpret_cast<const void*>(batch_iter->index_start * sizeof(uint)));
};

void UIOpenglBackend::processTextCommand(UIRenderCommand& command, float*& vertices, uint*& indices, int& index_offset){
    int font_size = command.size.x;
    int x = command.position.x;
    int y = command.position.y;
    std::string& text = command.text;

    float scale = static_cast<float>(command.size.x) / static_cast<float>(mainFont.getSize());
    //std::cout << scale << std::endl;
    glm::vec2 textDimensions = mainFont.getTextDimensions(text, font_size);

    // Iterate through each character in the text
    for (auto c = text.begin(); c != text.end(); c++) {
        Character ch = mainFont.getCharacters()[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - ch.Bearing.y * scale + textDimensions.y;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        UIRenderCommand glyph_command = {
            {
                static_cast<int>(xpos),
                static_cast<int>(ypos),
            },
            {
                static_cast<int>(w),
                static_cast<int>(h),
            },
            command.color,
            {
                {ch.TexCoordsMin.x},
                {ch.TexCoordsMin.y},
                {ch.TexCoordsMax.x - ch.TexCoordsMin.x},
                {ch.TexCoordsMax.y - ch.TexCoordsMin.y}
            },
            UIRenderCommand::GLYPH
        };

        proccessRenderCommand(glyph_command,vertices,indices,index_offset);

        // Advance to next glyph
        x += (ch.Advance >> 6) * scale;  // Bitshift by 6 to get the value in pixels
    }
}

void UIOpenglBackend::proccessRenderCommand(UIRenderCommand& command, float*& vertices, uint*& indices, int& index_offset){
    if(command.type == UIRenderCommand::TEXT){
        processTextCommand(command, vertices, indices, index_offset);
        return;
    }

    int x = command.position.x;
    int y = command.position.y;
    int width = command.size.x;
    int height = command.size.y;
    
    glm::vec2 vertices_[4] = {
        {x    , y    },
        {x + width, y    },
        {x + width, y + height},
        {x    , y + height}
    };

    float textureX = command.uvs.x;
    float textureY = command.uvs.y;
    float textureXW = textureX + command.uvs.width;
    float textureYH = textureY + command.uvs.height;

    glm::vec2 textureCoordinates[4] = {
        {textureX , textureY },
        {textureXW, textureY },
        {textureXW, textureYH},
        {textureX , textureYH}
    };

    //std::cout << "ZIndex: " << static_cast<float>(zIndex) / 100.0f << std::endl;
    //std::cout << borderSize.x << " " << borderSize.y << " " << borderSize.z << " " << borderSize.w << std::endl;
    uint vecIndices[4];
    uint startIndex =  index_offset;

    for(int i = 0; i < 4; i++){
        vertices[0] = vertices_[i].x;
        vertices[1] = vertices_[i].y;

        vertices[2] = command.color.r;
        vertices[3] = command.color.g;
        vertices[4] = command.color.b;
        vertices[5] = command.color.a;

        vertices[6] = textureCoordinates[i].x;
        vertices[7] = textureCoordinates[i].y;

        vertices[8] = static_cast<float>(command.type);

        vertices += vertex_size;

        vecIndices[i] = startIndex + i;
    }

    *indices++ = vecIndices[3];
    *indices++ = vecIndices[1];
    *indices++ = vecIndices[0];
    *indices++ = vecIndices[3];
    *indices++ = vecIndices[2];
    *indices++ = vecIndices[1];

    index_offset += 4;
}

void UIOpenglBackend::resizeVieport(int width, int height){
    projection_matrix = glm::ortho(
        0.0f,   // Left
        (float)width, // Right
        (float)height, // Top
        0.0f,   // Bottom
        -1.0f,  // Near plane
        1.0f    // Far plane
    );
}