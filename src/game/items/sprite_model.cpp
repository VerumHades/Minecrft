#include <game/items/sprite_model.hpp>

void SpriteModel::setupMesh(){
    std::unique_ptr<Mesh> mesh = generateFaces();
    if(!mesh) return;

    float thickness = (scale / 32.0f) * 2.0f;
    float texture_size = scale;

    std::array<glm::vec3,8> vertices = {
        glm::vec3(0.0f,texture_size,0.0f),
        glm::vec3(texture_size,texture_size,0.0f),
        glm::vec3(texture_size,0.0f,0.0f),
        glm::vec3(0.0f,0.0f,0.0f),
        glm::vec3(0.0f,texture_size,thickness),
        glm::vec3(texture_size,texture_size,thickness),
        glm::vec3(texture_size,0.0f,thickness),
        glm::vec3(0.0f,0.0f,thickness)
    };

    std::vector<float> metadata = {
        0, // Is not solid colored
        0,0,0,
        1 // Pixel perfect sampling enabled
    };

    mesh->addQuadFace({vertices[0],vertices[1],vertices[2],vertices[3]}, {0,0,-1}, true , metadata); // Front face
    mesh->addQuadFace({vertices[4],vertices[5],vertices[6],vertices[7]}, {0,0, 1}, false, metadata); // Back face

    mesh->setTexture(std::make_shared<GLTexture2D>(sprite_path.c_str()));

    addMesh(*mesh);

    rotation_center_offset = {-0.5,-0.5,0};
}
std::unique_ptr<Mesh> SpriteModel::generateFaces(){
    int width = 0, height = 0, original_channels = 0;
    unsigned char* image_data = nullptr;
    // Load the image with 4 channels (RGBA)
    image_data = stbi_load(sprite_path.c_str(), &width, &height, &original_channels, 4);

    if (!image_data) {
        std::cout << "Failed to load image: " << stbi_failure_reason() << std::endl;
        return nullptr;
    }
    auto mesh = std::make_unique<Mesh>(createMesh());

    const int size = std::min(width,height);
    float thickness = scale / (float)size;

    for(int y = 0;y < size;y++){
        for(int x = 0;x < size;x++){
            int y_inverted = size - y; // Invert y to flip the generated faces 

            auto current_pixel = getPixel(image_data, x    ,y_inverted    , width, height);
            auto left_pixel    = getPixel(image_data, x - 1,y_inverted    , width, height);
            auto upper_pixel   = getPixel(image_data, x    ,y_inverted - 1, width, height);
            auto right_pixel   = getPixel(image_data, x + 1,y_inverted    , width, height);
            auto bottom_pixel  = getPixel(image_data, x    ,y_inverted + 1, width, height);
            
            if(current_pixel.a == 0) continue;

            std::vector<float> metadata = {
                1, // Is solid colored
                static_cast<float>(current_pixel.r) / 255.0f,
                static_cast<float>(current_pixel.g) / 255.0f,
                static_cast<float>(current_pixel.b) / 255.0f,
                0 // Doesnt matter
            };

            if(left_pixel.a == 0){
                mesh->addQuadFace({
                    glm::vec3(x, y         , 0) * thickness,
                    glm::vec3(x, y         , 2) * thickness, 
                    glm::vec3(x,(y - 1)    , 2) * thickness,
                    glm::vec3(x,(y - 1)    , 0) * thickness
                }, {-1,0,0}, false, metadata);
            }
            if(upper_pixel.a == 0){
                mesh->addQuadFace({
                    glm::vec3(x          , y, 0) * thickness,
                    glm::vec3((x + 1)    , y, 0) * thickness,
                    glm::vec3((x + 1)    , y, 2) * thickness,
                    glm::vec3(x          , y, 2) * thickness
                }, {0,1,0}, false, metadata);
            }
            if(right_pixel.a == 0){
                mesh->addQuadFace({
                    glm::vec3((x + 1), y      , 0) * thickness,
                    glm::vec3((x + 1), (y - 1), 0) * thickness,
                    glm::vec3((x + 1), (y - 1), 2) * thickness,
                    glm::vec3((x + 1), y      , 2) * thickness
                }, {1,0,0}, false, metadata); 
            }
            if(bottom_pixel.a == 0){
                mesh->addQuadFace({
                    glm::vec3(x      , (y - 1), 0) * thickness,
                    glm::vec3((x + 1), (y - 1), 0) * thickness,
                    glm::vec3((x + 1), (y - 1), 2) * thickness,
                    glm::vec3(x      , (y - 1), 2) * thickness
                }, {0,-1,0}, true, metadata);
            }
        }
    }
    stbi_image_free(image_data);

    return mesh;
}