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
    
    auto frame = std::make_unique<UIFrame>(
        TValue(OPERATION_MINUS, {FRACTIONS,50}, {MFRACTION,50}),
        TValue(OPERATION_MINUS, {FRACTIONS,50}, {MFRACTION,50}),
        TValue(PIXELS,300),
        TValue(PIXELS,300),
        glm::vec3(0.5,0.5,0.9)
    );
    windows.push_back(std::move(frame));

    auto label = std::make_unique<UILabel>(
        "Hello World!", 
        TValue(PIXELS,30),
        TValue(PIXELS,30),
        glm::vec3(0.5,0.1,0.9)
    );
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

    update();
}

static const glm::vec2 textureCoordinates[4] = {{1, 1},{0, 1},{0, 0},{1, 0}};
void UIManager::processRenderingInformation(UIRenderInfo& info, UIFrame& frame, Mesh& output){
    int x = frame.getValueInPixels(info.x, true, screenWidth);
    int y = frame.getValueInPixels(info.y, false, screenHeight);
    int w = frame.getValueInPixels(info.width, true, screenWidth);
    int h = frame.getValueInPixels(info.height, false, screenHeight);

    glm::vec2 vertices_[4] = {
        {x    , y    },
        {x + w, y    },
        {x + w, y + h},
        {x    , y + h}
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

        vertex[7 + offset] = static_cast<float>(w);
        vertex[8 + offset] = static_cast<float>(h);

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
        for(auto& info: winfo) processRenderingInformation(info, *window, temp);
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
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale  + textDimensions.y;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        //ypos += textDimensions.y - h;

        out.push_back({
            {PIXELS, static_cast<int>(xpos)},
            {PIXELS, static_cast<int>(ypos)},
            {PIXELS, static_cast<int>(w)},
            {PIXELS, static_cast<int>(h)},
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

int UIFrame::getValueInPixels(TValue& value, bool horizontal, int container_size){
    switch (value.unit)
    {
        case PIXELS: return value.value;   
        case FRACTIONS: return (container_size / 100.0f) * value.value;
        case OPERATION_PLUS: return getValueInPixels(value.operands[0], horizontal, container_size) + getValueInPixels(value.operands[1], horizontal, container_size);
        case OPERATION_MINUS: return getValueInPixels(value.operands[0], horizontal, container_size) - getValueInPixels(value.operands[1], horizontal, container_size);
        case MFRACTION: 
            return static_cast<float>(horizontal ? getValueInPixels(width, horizontal, container_size) : getValueInPixels(height, horizontal, container_size)) / 100.0f * value.value;
    }
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
    glm::vec2 textDimensions = manager.getMainFont().getTextDimensions(text);
    int sw = manager.getScreenWidth();
    int sh = manager.getScreenHeight();

    int rxpadding = getValueInPixels(padding, true , sw);
    int rypadding = getValueInPixels(padding, false, sh);

    int rx = getValueInPixels(x, true , sw) + rxpadding;
    int ry = getValueInPixels(y, false, sh) + rypadding;
    
    int w = textDimensions.x + rxpadding * 2;
    int h = textDimensions.y + rypadding * 2;
    
    std::vector<UIRenderInfo> out = {
       {x,y,{PIXELS, w},{PIXELS, h},false,color}
    };
    //std::cout << "Building label" << std::endl;

    std::vector<UIRenderInfo> temp = manager.buildTextRenderingInformation(text,rx,ry,1,{1,1,1});
    out.insert(out.end(), temp.begin(), temp.end());

    return out;
}