#include <game/items/block_model.hpp>

std::array<glm::vec2, 4> BlockModel::getTextureCoordinates(int i){
    return {
        glm::vec2{(1.0f / 6.0f) * static_cast<float>(i    ), 0},
        glm::vec2{(1.0f / 6.0f) * static_cast<float>(i + 1), 0},
        glm::vec2{(1.0f / 6.0f) * static_cast<float>(i + 1), 1},
        glm::vec2{(1.0f / 6.0f) * static_cast<float>(i    ), 1}
    };
}

BlockModel::BlockModel(const BlockRegistry::BlockPrototype* prototype){
    texture = std::make_shared<GLTexture2D>();

    if(prototype->single_texture){
        Image image_loaded{prototype->texture_paths[0]};
        Image image = Image::perfectPixelReduce(image_loaded, 32, 32);
        texture->configure(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 32, 32);
        texture->putImage(0,0,image);
    }
    else{
        texture->configure(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 32 * 6, 32);
        for(int i = 0;i < 6;i++){
            Image image_loaded{prototype->texture_paths[i]};
            Image image = Image::perfectPixelReduce(image_loaded, 32, 32);
            texture->putImage(32 * i, 0, image);
        }
    }

    Mesh mesh{GLVertexFormat({VEC3, VEC3, VEC2})};

    std::array<glm::vec3,8> vertices = {
        glm::vec3(0.0f,1.0f,0.0f),
        glm::vec3(1.0f,1.0f,0.0f),
        glm::vec3(1.0f,0.0f,0.0f),
        glm::vec3(0.0f,0.0f,0.0f),
        glm::vec3(0.0f,1.0f,1.0f),
        glm::vec3(1.0f,1.0f,1.0f),
        glm::vec3(1.0f,0.0f,1.0f),
        glm::vec3(0.0f,0.0f,1.0f)
    };

    std::vector<float> metadata = {
        0, // Is not solid colored
        0,0,0,
        0
    };

    if(prototype->single_texture){
        mesh.addQuadFace({vertices[0],vertices[4],vertices[5],vertices[1]}, {0,1 ,0}, true , metadata); // Top face
        mesh.addQuadFace({vertices[2],vertices[6],vertices[7],vertices[3]}, {0,-1,0}, false, metadata); // Bottom face

        mesh.addQuadFace({vertices[0],vertices[4],vertices[7],vertices[3]}, {-1,0,0}, true , metadata); // Left face
        mesh.addQuadFace({vertices[5],vertices[1],vertices[2],vertices[6]}, {1 ,0,0}, false, metadata); // Right face

        mesh.addQuadFace({vertices[0],vertices[1],vertices[2],vertices[3]}, {0,0,-1}, true , metadata); // Front face
        mesh.addQuadFace({vertices[4],vertices[5],vertices[6],vertices[7]}, {0,0, 1}, false, metadata); // Back face
    }
    else{
        mesh.addQuadFace({vertices[0],vertices[4],vertices[5],vertices[1]}, {0,1 ,0}, true , metadata, getTextureCoordinates(0)); // Top face
        mesh.addQuadFace({vertices[2],vertices[6],vertices[7],vertices[3]}, {0,-1,0}, false, metadata, getTextureCoordinates(1)); // Bottom face

        mesh.addQuadFace({vertices[0],vertices[4],vertices[7],vertices[3]}, {-1,0,0}, true , metadata, getTextureCoordinates(2)); // Left face
        mesh.addQuadFace({vertices[5],vertices[1],vertices[2],vertices[6]}, {1 ,0,0}, false, metadata, getTextureCoordinates(3)); // Right face

        mesh.addQuadFace({vertices[0],vertices[1],vertices[2],vertices[3]}, {0,0,-1}, true , metadata, getTextureCoordinates(4)); // Front face
        mesh.addQuadFace({vertices[4],vertices[5],vertices[6],vertices[7]}, {0,0, 1}, false, metadata, getTextureCoordinates(5)); // Back face
    }

    rotation_center_offset = {-0.5,-0.5,-0.5};

    vertex_buffer.initialize(mesh.getVertices().size());
    vertex_buffer.insert(0, mesh.getVertices().size(), mesh.getVertices().data());

    index_buffer.initialize(mesh.getIndices().size());
    index_buffer.insert(0, mesh.getIndices().size(), mesh.getIndices().data());
}