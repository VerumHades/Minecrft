#include <ui/opengl_backend.hpp>

UIOpenglBackend::UIOpenglBackend(){
    shader_program.use();
    fontManager.initialize();

    vao.bind();

    vao.attachBuffer(&vertex_buffer, {VEC2,VEC4,VEC2,FLOAT});
    vertex_buffer.initialize(1);
    vao.attachBuffer(&index_buffer);
    index_buffer.initialize(1);

    vao.unbind();


    shader_program.setSamplerSlot("tex",0);
    shader_program.setSamplerSlot("textAtlas",1);
}

void UIOpenglBackend::setupRender(){
    if(needs_update){
        if(index_buffer.size() != indices.size()) index_buffer.initialize(indices.size(), indices.data());
        else index_buffer.insert(0, indices.size(), indices.data());

        if(vertex_buffer.size() != vertices.size()) vertex_buffer.initialize(vertices.size(), vertices.data());
        else vertex_buffer.insert(0, vertices.size(), vertices.data());

        needs_update = false;
    }

    GL_CALL( glDisable(GL_CULL_FACE));
    GL_CALL( glDisable(GL_DEPTH_TEST));
    GL_CALL( glEnable(GL_BLEND));
    GL_CALL( glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    //glEnable(GL_SCISSOR_TEST);
    //glClear(GL_DEPTH_BUFFER_BIT);

    shader_program.updateUniforms();

    vao.bind();
    mainFont.getAtlas()->bind(1);
}

void UIOpenglBackend::cleanupRender(){
    GL_CALL( glDisable(GL_SCISSOR_TEST));
    GL_CALL( glDisable(GL_BLEND));
    GL_CALL( glEnable(GL_DEPTH_TEST));
    GL_CALL( glEnable(GL_CULL_FACE));
    vao.unbind();
}

std::tuple<size_t, size_t> UIOpenglBackend::calculateBatchSizes(UIRenderBatch& batch){
    size_t total_commands = 0;

    for(auto& command: batch.commands){
        if(command.type == UIRenderCommand::TEXT){
            total_commands += command.text.size();
            continue;
        }
        
        total_commands++;
    }

    return {
        total_commands * 4 * vertex_size,
        total_commands * 6
    };
}

std::list<UIBackend::Batch>::iterator UIOpenglBackend::addRenderBatch(UIRenderBatch& batch){
    auto [vertex_count, index_count] = calculateBatchSizes(batch);

    if(vertex_count == 0)
        return batches.insert(batches.end(), {
            batch.clipRegion,
            batch.texture,
            0,
            0,
            0,
            0
        });
        
    size_t vertex_start = vertices.insert(nullptr, vertex_count);
    size_t index_start = indices.insert(nullptr, index_count);

    float* vertex_ptr = vertices.data() + vertex_start;
    uint* index_ptr = indices.data() + index_start;

    int index_offset = vertex_start / vertex_size;
    for(auto& command: batch.commands){
        proccessRenderCommand(command, vertex_ptr, index_ptr, index_offset);
    }

    if(
        (vertex_ptr - (vertices.data() + vertex_start)) != vertex_count ||
        (index_ptr - (indices.data() + index_start)) !=  index_count
    ){
        throw std::runtime_error("Batch overeach! This is an internal bug.");
    }

    needs_update = true;

    return batches.insert(batches.end(), {
        batch.clipRegion,
        batch.texture,
        vertex_start,
        index_start,
        vertex_count,
        index_count
    });
}

void UIOpenglBackend::removeBatch(std::list<UIBackend::Batch>::iterator batch_iter){
    if(batch_iter->vertex_size == 0) {
        batches.erase(batch_iter);
        return;
    }
    vertices.free(batch_iter->vertex_start);
    indices.free(batch_iter->index_start);
    batches.erase(batch_iter);
    needs_update = true;
}

void UIOpenglBackend::renderBatch(std::list<UIBackend::Batch>::iterator batch_iter){
    if(batch_iter->texture) batch_iter->texture->bind(0);
    /*glScissor(
        batch_iter->clipRegion.min.x,
        batch_iter->clipRegion.min.y,
        batch_iter->clipRegion.max.x - batch_iter->clipRegion.min.x,
        batch_iter->clipRegion.max.y - batch_iter->clipRegion.min.y
    );*/
    GL_CALL( glDrawElements(GL_TRIANGLES, batch_iter->index_size, GL_UNSIGNED_INT, reinterpret_cast<const void*>(batch_iter->index_start * sizeof(uint))));
};

void UIOpenglBackend::processTextCommand(UIRenderCommand& command, float*& vertices, uint*& indices, int& index_offset){
    int font_size = command.vertex_positions[1].x;
    int x = command.vertex_positions[0].x;
    int y = command.vertex_positions[0].y;
    std::string& text = command.text;

    float scale = static_cast<float>(font_size) / static_cast<float>(mainFont.getSize());
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
            UIRenderBatch::GetRetangleVertices(xpos,ypos,w,h),
            command.color,
            UIRegion{
                ch.TexCoordsMin,
                ch.TexCoordsMax
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

    /*int x = command.position.x;
    int y = command.position.y;
    int width = command.size.x;
    int height = command.size.y;
    
    glm::vec2 vertices_[4] = {
        {x    , y    },
        {x + width, y    },
        {x + width, y + height},
        {x    , y + height}
    };*/

    float textureX = command.uvs.min.x;
    float textureY = command.uvs.min.y;
    float textureXW = command.uvs.max.x;
    float textureYH = command.uvs.max.y;

    //std::cout << textureX << " " << textureY << " " << textureXW << " " << textureYH << std::endl;

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
        vertices[0] = command.vertex_positions[i].x;
        vertices[1] = command.vertex_positions[i].y;

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

UITextDimensions UIOpenglBackend::getTextDimensions(std::string text, int size) {
    auto dimensions = mainFont.getTextDimensions(text,size);
    return UITextDimensions{static_cast<int>(dimensions.x), static_cast<int>(dimensions.y)};
}