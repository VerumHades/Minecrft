#include <ui/manager.hpp>

void UIManager::initialize(){
    uiProgram = std::make_unique<ShaderProgram>();
    uiProgram->initialize();
    uiProgram->addShader("shaders/ui/ui.vs", GL_VERTEX_SHADER);
    uiProgram->addShader("shaders/ui/ui.fs", GL_FRAGMENT_SHADER);
    uiProgram->compile();
    uiProgram->use();

    drawBuffer = std::make_unique<GLBuffer>();

    projectionMatrix.attach(*uiProgram);

    resize(1920,1080);

    windows.push_back({20,20,200,200, {0.1,0.5,0.9}});
}

void UIManager::resize(int width, int height){
    screenWidth = width;
    screenHeight = height;

    projectionMatrix = glm::ortho(
        0.0f,   // Left
        (float)width, // Right
        (float)height, // Top
        0.0f,   // Bottom
        -1.0f,  // Near plane
        1.0f    // Far plane
    );
}

static const glm::vec2 textureCoordinates[4] = {{1, 1},{0, 1},{0, 0},{1, 0}};
void UIManager::processRenderingInformation(UIRenderInfo& info, Mesh& output){
    glm::vec2 vertices_[4] = {
        {info.x             , info.y         },
        {info.x + info.width, info.y         },
        {info.x + info.width, info.y + info.height},
        {info.x             , info.y + info.height}
    };
    
    uint32_t vecIndices[4];

    const int vertexSize = 9;

    float vertex[vertexSize * 4];
    uint32_t startIndex = (uint32_t) output.getVertices().size() / vertexSize;
    for(int i = 0; i < 4; i++){
        int offset = i * vertexSize;

        vertex[0 + offset] = vertices_[i].x;
        vertex[1 + offset] = vertices_[i].y;

        vertex[2 + offset] = textureCoordinates[i].x;
        vertex[3 + offset] = textureCoordinates[i].y;

        vertex[4 + offset] = info.color.x;
        vertex[5 + offset] = info.color.y;
        vertex[6 + offset] = info.color.z;

        vertex[7 + offset] = static_cast<float>(info.width);
        vertex[8 + offset] = static_cast<float>(info.height);

        vecIndices[i] = startIndex + i;
    }

    output.getVertices().insert(output.getVertices().end(), vertex, vertex + vertexSize * 4);
    output.getIndices().insert(output.getIndices().end(), {vecIndices[3], vecIndices[1], vecIndices[0], vecIndices[3], vecIndices[2], vecIndices[1]});

    //std::cout << info.x << " " << info.y << " " << info.width << " " << info.height << std::endl;
}

void UIManager::update(){
    uiProgram->use();

    Mesh temp = Mesh();
    temp.setVertexFormat({2,2,3,2});

    for(auto& window: windows){
        std::vector<UIRenderInfo> winfo = window.getRenderingInformation();
        for(auto& info: winfo) processRenderingInformation(info, temp);
    }

    drawBuffer->loadMesh(temp);
}

void UIManager::draw(){
    uiProgram->updateUniforms();
    drawBuffer->draw();
}

void UIManager::mouseMove(int x, int y){

}

std::vector<UIRenderInfo> UIFrame::getRenderingInformation(){
    std::vector<UIRenderInfo> out = {{
        x,y,width,height,color
    }};

    for(auto& child: children){
        auto temp = child.getRenderingInformation();
        out.insert(out.end(), temp.begin(), temp.end());
    }

    return out;
};