#include <rendering/cubes.hpp>


void CubeRenderer::loadTextures(const std::vector<std::string>& textures){
    texture_count = textures.size();

    loaded_texture = std::make_shared<GLTexture2D>();
    
    loaded_texture->configure(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 32 * texture_count.getValue(), 32);
    for(size_t i = 0;i < textures.size();i++){
        Image image = Image::LoadWithSize(textures[i], 32, 32);
        loaded_texture->putImage(32 * i, 0, image);
    }
}
CubeRenderer::CubeRenderer(){
    Mesh mesh = Mesh({VEC3,VEC3,VEC2});
    
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

    mesh.addQuadFace({vertices[0],vertices[4],vertices[5],vertices[1]}, {0,1 ,0}, true , {}); // Top face
    mesh.addQuadFace({vertices[2],vertices[6],vertices[7],vertices[3]}, {0,-1,0}, false, {}); // Bottom face

    mesh.addQuadFace({vertices[0],vertices[4],vertices[7],vertices[3]}, {-1,0,0}, true , {}); // Left face
    mesh.addQuadFace({vertices[5],vertices[1],vertices[2],vertices[6]}, {1 ,0,0}, false, {}); // Right face

    mesh.addQuadFace({vertices[0],vertices[1],vertices[2],vertices[3]}, {0,0,-1}, true , {}); // Front face
    mesh.addQuadFace({vertices[4],vertices[5],vertices[6],vertices[7]}, {0,0, 1}, false, {}); // Back face

    loaded_mesh = mesh.load();
    
    instanceBuffer.initialize(maxCubes * 4);
    instanceBuffer.bind();

    loaded_mesh->getVAO().attachBuffer(&instanceBuffer, {{VEC3, FLOAT}, true}, 0);

    program.setSamplerSlot("cube_texture_atlas", 0);
}

void CubeRenderer::setCube(size_t index, glm::vec3 position, int texture){
    float data[4];
    std::memcpy(data, glm::value_ptr(position), 3 * sizeof(float));
    data[3] = (float)texture;

    instanceBuffer.insert(index * 4, 4, data);
    cubes = std::max(cubes, index + 1);
}

void CubeRenderer::removeCube(size_t index){
    float data[4] = {0,0,0,0};
    instanceBuffer.insert(index * 4, 4, data);
}

void CubeRenderer::draw(){
    GLBinding vao_binding(loaded_mesh->getVAO());

    loaded_texture->bind(0);

    program.updateUniforms();
    GL_CALL( glDrawElementsInstanced(GL_TRIANGLES, loaded_mesh->getIndexBuffer().size(), GL_UNSIGNED_INT, 0, cubes));
}