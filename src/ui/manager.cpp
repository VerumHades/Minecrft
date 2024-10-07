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
    UIImage::textures = std::make_unique<DynamicTextureArray>();

    projectionMatrix.attach(uiProgram);
    projectionMatrix.attach(fontManager.getProgram());

    glUniform1i(uiProgram.getUniformLocation("tex"),0);
    glUniform1i(uiProgram.getUniformLocation("textAtlas"),1);

    resize(1920,1080);
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
    int x = info.x;
    int y = info.y;
    int w = info.width;
    int h = info.height;

    glm::vec2 vertices_[4] = {
        {x    , y    },
        {x + w, y    },
        {x + w, y + h},
        {x    , y + h}
    };

    uint32_t vecIndices[4];

    const int vertexSize = 21;

    glm::vec4 borderSize = {
        info.borderWidth.top,
        info.borderWidth.right,
        info.borderWidth.bottom,
        info.borderWidth.left
    };

    // Border width recalculated to be relative to the elements dimensions
    if(borderSize.x != 0) borderSize.x /= static_cast<float>(h);
    if(borderSize.y != 0) borderSize.y /= static_cast<float>(w);
    if(borderSize.z != 0) borderSize.z /= static_cast<float>(h);
    if(borderSize.w != 0) borderSize.w /= static_cast<float>(w);

    //std::cout << borderSize.x << " " << borderSize.y << " " << borderSize.z << " " << borderSize.w << std::endl;

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

        vertex[4 + offset] = info.color.r;
        vertex[5 + offset] = info.color.g;
        vertex[6 + offset] = info.color.b;
        vertex[7 + offset] = info.color.a;

        vertex[8 + offset] = static_cast<float>(w);
        vertex[9 + offset] = static_cast<float>(h);

        vertex[10 + offset] = info.isText ? 1.0 : 0.0;
        vertex[11 + offset] = info.isTexture ? 1.0 : 0.0;
        vertex[12 + offset] = static_cast<float>(info.textureIndex);

        vertex[13 + offset] = borderSize.x;
        vertex[14 + offset] = borderSize.y;
        vertex[15 + offset] = borderSize.z;
        vertex[16 + offset] = borderSize.w;

        vertex[17 + offset] = info.borderColor.r;
        vertex[18 + offset] = info.borderColor.g;
        vertex[19 + offset] = info.borderColor.b;
        vertex[20 + offset] = info.borderColor.a;

        vecIndices[i] = startIndex + i;
    }

    output.getVertices().insert(output.getVertices().end(), vertex, vertex + vertexSize * 4);
    output.getIndices().insert(output.getIndices().end(), {vecIndices[3], vecIndices[1], vecIndices[0], vecIndices[3], vecIndices[2], vecIndices[1]});

    //std::cout << info.x << " " << info.y << " " << info.width << " " << info.height << std::endl;
}

void UIManager::update(){
    if(currentWindow == (UIWindowIdentifier)-1) return;

    //std::cout << getCurrentWindow().getCurrentLayer().getElements().size() << std::endl;

    uiProgram.use();

    Mesh temp = Mesh();
    temp.setVertexFormat({2,2,4,2,1,1,1,4,4});

    for(auto& window: getCurrentWindow().getCurrentLayer().getElements()){
        std::vector<UIRenderInfo> winfo = window->getRenderingInformation(*this);
        for(auto& info: winfo) processRenderingInformation(info, *window, temp);
    }

    drawBuffer->loadMesh(temp);
}

std::vector<UIRenderInfo> UIManager::buildTextRenderingInformation(std::string text, float x, float y, float scale, UIColor color){
    std::vector<UIRenderInfo> out = {};

    glm::vec2 textDimensions = mainFont->getTextDimensions(text);

    // Iterate through each character in the text
    for (auto c = text.begin(); c != text.end(); c++) {
        Character ch = mainFont->getCharacters()[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - ch.Bearing.y * scale + textDimensions.y;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        //ypos += textDimensions.y - h;

        out.push_back(UIRenderInfo::Text(
            static_cast<int>(xpos),
            static_cast<int>(ypos),
            static_cast<int>(w),
            static_cast<int>(h),
            color,
            {
                {ch.TexCoordsMin.x, ch.TexCoordsMin.y},
                {ch.TexCoordsMax.x, ch.TexCoordsMin.y},
                {ch.TexCoordsMax.x, ch.TexCoordsMax.y},
                {ch.TexCoordsMin.x, ch.TexCoordsMax.y}
            }
        ));

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
        case PFRACTION:
            if(parent){
                return static_cast<float>(horizontal ? getValueInPixels(parent->width, horizontal, container_size) : getValueInPixels(parent->height, horizontal, container_size)) / 100.0f * value.value;
            }
            else return (container_size / 100.0f) * value.value; // Fall back to container size
    }
}

void UIManager::render(){
    glDisable( GL_CULL_FACE );
    glDisable(GL_DEPTH_TEST);

    uiProgram.updateUniforms();
    UIImage::textures->bind(0);
    mainFont->getAtlas()->bind(1);
    drawBuffer->draw();
}

void UIManager::mouseMove(int x, int y){
    UIFrame* element = getElementUnder(x,y);
    if(element != underHover && underHover != nullptr) underHover->setHover(false);   
    if(element != nullptr) element->setHover(true);
    underHover = element;
    update();
}

void UIManager::mouseEvent(GLFWwindow* window, int button, int action, int mods){
    if(!underHover) return;
    if(underHover->onMouseEvent) underHover->onMouseEvent(window,button,action,mods);
    update();
}

void UIManager::keyTypedEvent(GLFWwindow* window, unsigned int codepoint){
    if(!inFocus) return;
    if(inFocus->onKeyTyped) inFocus->onKeyTyped(window,codepoint);
    update();
}
void UIManager::keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(!inFocus) return;
    if(inFocus->onKeyEvent) inFocus->onKeyEvent(window,key,scancode,action,mods);
    update();
}

void UIManager::setCurrentWindow(UIWindowIdentifier id){
    inFocus = nullptr;
    underHover = nullptr;
    currentWindow = id;
    update();
}
UIWindow& UIManager::getCurrentWindow(){
    return windows[currentWindow];
}
UIWindow& UIManager::getWindow(UIWindowIdentifier id){
    return windows[id];
}
UIWindowIdentifier UIManager::createWindow(){
    windows[lastWindowIndentifier] = {};
    return lastWindowIndentifier++;
}

UIFrame* UIManager::getElementUnder(int x, int y){
    if(currentWindow == (UIWindowIdentifier)-1) return nullptr;

    std::queue<std::tuple<int, UIFrame*, UIFrame*>> elements;
    
    for(auto& window: getCurrentWindow().getCurrentLayer().getElements()) elements.push({0,window.get(), nullptr});
    

    UIFrame* current = nullptr;
    int cdepth = -1;

    while(!elements.empty()){
        auto [depth,element,parent] = elements.front();
        elements.pop();

        for(auto& child: element->getChildren()) elements.push({depth + 1, child.get(), element});

        if(cdepth >= depth) continue;
        if(!element->pointWithin({x,y}, *this)) continue;
        current = element;
    }

    return current;
}

bool UIFrame::pointWithin(glm::vec2 point, UIManager& manager){
    int sw = manager.getScreenWidth();
    int sh = manager.getScreenHeight();

    auto t = getTransform(manager);

    return  point.x > t.x && point.x < t.x + t.width &&
            point.y > t.y && point.y < t.y + t.height;
}

std::vector<UIRenderInfo> UIFrame::getRenderingInformation(UIManager& manager){
    auto clr = hover ? hoverColor : color;
    
    auto t = getTransform(manager);

    std::vector<UIRenderInfo> out = {
        UIRenderInfo::Rectangle(t.x,t.y,t.width,t.height,clr,getBorderSizes(manager),borderColor)
    };

    for(auto& child: children){
        auto temp = child->getRenderingInformation(manager);
        out.insert(out.end(), temp.begin(), temp.end());
    }

    return out;
};

UITransform UIFrame::getTransform(UIManager& manager){
    int sw = manager.getScreenWidth();
    int sh = manager.getScreenHeight();

    int ox = 0, oy = 0;
    if(parent){
        auto t = parent->getTransform(manager);
        ox = t.x;
        oy = t.y;
    }

    return {
        getValueInPixels(x     , true , sw) + ox,
        getValueInPixels(y     , false, sh) + oy,
        getValueInPixels(width , true , sw),
        getValueInPixels(height, false, sh),
    };
}
UIBorderSizes UIFrame::getBorderSizes(UIManager& manager){
    int sw = manager.getScreenWidth();
    int sh = manager.getScreenHeight();

    return {
        getValueInPixels(borderWidth[0], false, sh),
        getValueInPixels(borderWidth[1], true , sw),
        getValueInPixels(borderWidth[2], false, sh),
        getValueInPixels(borderWidth[3], true , sw)
    };
}

std::vector<UIRenderInfo> UILabel::getRenderingInformation(UIManager& manager) {
    glm::vec2 textDimensions = manager.getMainFont().getTextDimensions(text);

    auto t = getTransform(manager);

    int tx = t.x + t.width  / 2 - textDimensions.x / 2;
    int ty = t.y + t.height / 2 - textDimensions.y / 2;
    
    std::vector<UIRenderInfo> out = UIFrame::getRenderingInformation(manager);
    std::vector<UIRenderInfo> temp = manager.buildTextRenderingInformation(text,tx,ty,1,{1,1,1,1});
    out.insert(out.end(), temp.begin(), temp.end());

    return out;
}

std::unique_ptr<DynamicTextureArray> UIImage::textures;

std::vector<UIRenderInfo> UIImage::getRenderingInformation(UIManager& manager){
    auto t = getTransform(manager);

    std::vector<UIRenderInfo> out = {UIRenderInfo::Texture(
        t.x,t.y,t.width,t.height,
        textures->getTextureUVs(path),
        textures->getTextureIndex(path)
    )};

    return out;
}

UIImage::UIImage(std::string path, TValue x, TValue y, TValue width, TValue height) : UIFrame(x,y,width,height,{0,0,0,0}), path(path){
    textures->addTexture(path);
}

UIInput::UIInput(TValue x, TValue y, TValue width, TValue height, UIColor color): UILabel("",x,y,width,height,color){
    this->width = width;
    this->height = height;

    onKeyTyped = [this](GLFWwindow* window, unsigned int codepoint){
        char typedChar = static_cast<char>(codepoint);

        if (typedChar >= 32 && typedChar <= 126) {
            this->text += typedChar;
        }
    };

    onKeyEvent = [this](GLFWwindow* window, int key, int scancode, int action, int mods){
        if(key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS){
            this->text = this->text.substr(0, this->text.size() - 1);
        }

        if(key == GLFW_KEY_ENTER && action == GLFW_PRESS){
            if(this->onSubmit) this->onSubmit(this->text);
        }
    };
}

std::vector<UIRenderInfo> UIInput::getRenderingInformation(UIManager& manager){
    glm::vec2 textDimensions = manager.getMainFont().getTextDimensions(text);

    auto t = getTransform(manager);

    int tx = t.x + t.width  / 2 - textDimensions.x / 2;
    int ty = t.y + t.height / 2 - textDimensions.y / 2;
    
    std::vector<UIRenderInfo> out = UIFrame::getRenderingInformation(manager);
    std::vector<UIRenderInfo> temp = manager.buildTextRenderingInformation(text,tx,ty,1,{1,1,1,1});
    out.insert(out.end(), temp.begin(), temp.end());

    return out;
}