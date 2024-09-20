#include <ui/manager.hpp>

void UIManager::initialize(){
    uiProgram.initialize();
    uiProgram.addShader("shaders/ui/ui.vs", GL_VERTEX_SHADER);
    uiProgram.addShader("shaders/ui/ui.fs", GL_FRAGMENT_SHADER);
    uiProgram.compile();
    uiProgram.use();

    fontManager.initialize();

    drawBuffer = std::make_unique<GLBuffer>();
    mainFont = std::make_unique<Font>("fonts/JetBrainsMono/fonts/variable/JetBrainsMono[wght].ttf", 24);

    projectionMatrix.attach(uiProgram);
    projectionMatrix.attach(fontManager.getProgram());

    glUniform1i(uiProgram.getUniformLocation("tex"),0);
    glUniform1i(uiProgram.getUniformLocation("textAtlas"),1);

    resize(1920,1080);
    
    auto frame = std::make_unique<UIFrame>(10,10,200,30,glm::vec3(0.5,0.5,0.9));
    windows.push_back(std::move(frame));

    auto label = std::make_unique<UILabel>("Hello World!", 10,10,200,30,glm::vec3(0.5,0.5,0.9));
    windows.push_back(std::move(label));
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

    const int vertexSize = 10;

    float vertex[vertexSize * 4];
    uint32_t startIndex = (uint32_t) output.getVertices().size() / vertexSize;
    for(int i = 0; i < 4; i++){
        int offset = i * vertexSize;

        vertex[0 + offset] = vertices_[i].x;
        vertex[1 + offset] = vertices_[i].y;

        if(info.hasTexCoords){
            vertex[2 + offset] = info.texCoords[i].x;
            vertex[3 + offset] = info.texCoords[i].y;
        }
        else{
            vertex[2 + offset] = textureCoordinates[i].x;
            vertex[3 + offset] = textureCoordinates[i].y;
        }

        vertex[4 + offset] = info.color.x;
        vertex[5 + offset] = info.color.y;
        vertex[6 + offset] = info.color.z;

        vertex[7 + offset] = static_cast<float>(info.width);
        vertex[8 + offset] = static_cast<float>(info.height);

        vertex[9 + offset] = info.isText ? 1.0 : 0.0;
    
        vecIndices[i] = startIndex + i;
    }

    output.getVertices().insert(output.getVertices().end(), vertex, vertex + vertexSize * 4);
    output.getIndices().insert(output.getIndices().end(), {vecIndices[3], vecIndices[1], vecIndices[0], vecIndices[3], vecIndices[2], vecIndices[1]});

    //std::cout << info.x << " " << info.y << " " << info.width << " " << info.height << std::endl;
}

void UIManager::update(){
    uiProgram.use();

    Mesh temp = Mesh();
    temp.setVertexFormat({2,2,3,2,1});

    for(auto& window: windows){
        std::vector<UIRenderInfo> winfo = window->getRenderingInformation(*this);
        for(auto& info: winfo) processRenderingInformation(info, temp);
    }

    drawBuffer->loadMesh(temp);
}

std::vector<UIRenderInfo> UIManager::buildTextRenderingInformation(std::string text, float x, float y, float scale, glm::vec3 color){
    std::vector<UIRenderInfo> out = {};

    glm::vec2 textDimensions = mainFont->getTextDimensions(text);

    // Iterate through each character in the text
    for (auto c = text.begin(); c != text.end(); c++) {
        Character ch = mainFont->getCharacters()[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        ypos += textDimensions.y - h;

        out.push_back({
            static_cast<int>(xpos),
            static_cast<int>(ypos),
            static_cast<int>(w),
            static_cast<int>(h),
            true, // Is text
            color,
            true, // Has tex coords
            {
                {ch.TexCoordsMin.x, ch.TexCoordsMin.y},
                {ch.TexCoordsMax.x, ch.TexCoordsMin.y},
                {ch.TexCoordsMax.x, ch.TexCoordsMax.y},
                {ch.TexCoordsMin.x, ch.TexCoordsMax.y}
            }
        });

        // Advance to next glyph
        x += (ch.Advance >> 6) * scale;  // Bitshift by 6 to get the value in pixels
    }

    return out;
}

void UIManager::draw(){
    uiProgram.updateUniforms();
    mainFont->getAtlas()->bind(1);
    drawBuffer->draw();
}

void UIManager::mouseMove(int x, int y){

}

std::vector<UIRenderInfo> UIFrame::getRenderingInformation(UIManager& manager){
    std::vector<UIRenderInfo> out = {
        {x,y,width,height,false,color}
    };

    for(auto& child: children){
        auto temp = child->getRenderingInformation(manager);
        out.insert(out.end(), temp.begin(), temp.end());
    }

    return out;
};

std::vector<UIRenderInfo> UILabel::getRenderingInformation(UIManager& manager) {
    //std::vector<UIRenderInfo> out = {
     //   {x,y,width,height,false,color}
    //};
    std::cout << "Building label" << std::endl;

    std::vector<UIRenderInfo> out = manager.buildTextRenderingInformation(text,x+5,y+5,1,{1,1,1});
    //out.insert(out.end(), temp.begin(), temp.end());

    return out;
}