#include <ui/manager.hpp>

void UIManager::initialize(){
    uiProgram.initialize();
    uiProgram.addShader("shaders/ui/ui.vs", GL_VERTEX_SHADER);
    uiProgram.addShader("shaders/ui/ui.fs", GL_FRAGMENT_SHADER);
    uiProgram.compile();
    uiProgram.use();

    fontManager.initialize();

    drawBuffer = std::make_unique<GLStripBuffer>(VertexFormat({2,2,4,2,1,1,1,4,4,4,4,4,4}));
    mainFont = std::make_unique<Font>("fonts/JetBrainsMono/fonts/variable/JetBrainsMono[wght].ttf", 24);
    textures = std::make_unique<DynamicTextureArray>();

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

static const glm::vec2 textureCoordinates[4] = {{0, 1},{1, 1},{1, 0},{0, 0}};
const int vertexSize = 3 + 2 + 4 + 2 + 1 + 1 + 1 + 4 + 4 + 4 + 4 + 4 + 4;

bool UIRenderInfo::valid(){
    return clipRegion.min.x < clipRegion.max.x && clipRegion.min.y < clipRegion.max.y;
}

RawRenderInfo UIRenderInfo::process(){
    if(!clip){
        clipRegion.min.x = x;
        clipRegion.min.y = y;
        clipRegion.max.x = x + width;
        clipRegion.max.y = y + height;
    }

    glm::vec2 vertices_[4] = {
        {x    , y    },
        {x + width, y    },
        {x + width, y + height},
        {x    , y + height}
    };

    glm::vec4 borderSize = {
        borderWidth.top,
        borderWidth.right,
        borderWidth.bottom,
        borderWidth.left
    };

    // Border width recalculated to be relative to the elements dimensions
    if(borderSize.x != 0) borderSize.x /= static_cast<float>(height);
    if(borderSize.y != 0) borderSize.y /= static_cast<float>(width);
    if(borderSize.z != 0) borderSize.z /= static_cast<float>(height);
    if(borderSize.w != 0) borderSize.w /= static_cast<float>(width);

    //std::cout << borderSize.x << " " << borderSize.y << " " << borderSize.z << " " << borderSize.w << std::endl;

    RawRenderInfo vertex;
    for(int i = 0; i < 4; i++){
        int offset = i * vertexSize;

        vertex[0 + offset] = vertices_[i].x;
        vertex[1 + offset] = vertices_[i].y;
        vertex[2 + offset] = zIndex;

        if(hasTexCoords){
            vertex[3 + offset] = texCoords[i].x;
            vertex[4 + offset] = texCoords[i].y;
        }
        else{
            vertex[3 + offset] = textureCoordinates[i].x;
            vertex[4 + offset] = textureCoordinates[i].y;
        }

        vertex[5 + offset] = color.r;
        vertex[6 + offset] = color.g;
        vertex[7 + offset] = color.b;
        vertex[8 + offset] = color.a;

        vertex[9 + offset] = static_cast<float>(width);
        vertex[10 + offset] = static_cast<float>(height);

        vertex[11 + offset] = isText ? 1.0 : 0.0;
        vertex[12 + offset] = isTexture ? 1.0 : 0.0;
        vertex[13 + offset] = static_cast<float>(textureIndex);

        vertex[14 + offset] = borderSize.x;
        vertex[15 + offset] = borderSize.y;
        vertex[16 + offset] = borderSize.z;
        vertex[17 + offset] = borderSize.w;

        vertex[18 + offset] = clipRegion.min.x;
        vertex[19 + offset] = clipRegion.min.y;
        vertex[20 + offset] = clipRegion.max.x;
        vertex[21 + offset] = clipRegion.max.y;

        for(int j  = 0;j < 4; j++){
            vertex[22 + offset + j * 4] = borderColor[j].r;
            vertex[23 + offset + j * 4] = borderColor[j].g;
            vertex[24 + offset + j * 4] = borderColor[j].b;
            vertex[25 + offset + j * 4] = borderColor[j].a;
        }
    }

    return vertex;    //std::cout << x << " " << y << " " << width << " " << height << std::endl;
}

size_t UIManager::appendRenderInfo(const RawRenderInfo& info){
    auto allocation = renderAllocator.allocate();

    if(allocation.failed){
        size_t size = renderAllocator.getMemSize();
        renderAllocator.increaseSize(info.size());
        allocation = renderAllocator.allocate();
    }

    drawBuffer->insertData(allocation.location, info.data(), info.size());
    return allocation.location;
}

std::vector<size_t> UIManager::appendRenderInfo(std::vector<RawRenderInfo> info){
    std::vector<size_t> outputPositions = {};
    outputPositions.reserve(info.size());
    
    for(auto& call: info){
        outputPositions.push_back(appendRenderInfo(call));
    }
    
    return outputPositions;
}

void UIManager::updateRenderInfo(size_t location, const RawRenderInfo& info){
    drawBuffer->insertData(location, info.data(), info.size());
    renderAllocator.free(location);
}

void UIManager::clearRenderInfo(size_t location){
    const RawRenderInfo zeroes = {0};

    drawBuffer->insertData(location, zeroes.data(), zeroes.size());
    renderAllocator.free(location);
}

void UIManager::clearRenderInfo(std::vector<size_t> locations){
    for(auto& location: locations) clearRenderInfo(location);
}

void UIManager::update(){
    if(currentWindow == (UIWindowIdentifier)-1) return;

    //std::cout << getCurrentWindow().getCurrentLayer().getElements().size() << std::endl;

    uiProgram.use();

    for(auto& element: getCurrentWindow().getCurrentLayer().getElements()){
        element->updateChildRenderData();
    }

    //drawBuffer->loadMesh(temp);
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

int UIFrame::getValueInPixels(TValue value, bool horizontal){
    switch (value.unit)
    {
        case NONE: return 0;
        case PIXELS: return value.value;   

        case WINDOW_WIDTH : return (manager.getScreenWidth()  / 100.0f) * value.value;
        case WINDOW_HEIGHT: return (manager.getScreenHeight() / 100.0f) * value.value;
        
        case OPERATION_PLUS    : return getValueInPixels(value.operands[0], horizontal) + getValueInPixels(value.operands[1], horizontal);
        case OPERATION_MINUS   : return getValueInPixels(value.operands[0], horizontal) - getValueInPixels(value.operands[1], horizontal);
        case OPERATION_MULTIPLY: return getValueInPixels(value.operands[0], horizontal) * getValueInPixels(value.operands[1], horizontal);
        case OPERATION_DIVIDE  : return getValueInPixels(value.operands[0], horizontal) / getValueInPixels(value.operands[1], horizontal);

        case MY_PERCENT: 
            return static_cast<float>(
                horizontal ? 
                getValueInPixels(width,  horizontal) : 
                getValueInPixels(height, horizontal)
            ) / 100.0f * value.value;

        case PERCENT:
            if(parent){
                auto t = parent->contentTransform;
                return static_cast<float>(horizontal ? 
                    t.width : 
                    t.height
                ) / 100.0f * value.value;
            }
            else return (( horizontal ? manager.getScreenWidth() : manager.getScreenHeight() )  / 100.0f) * value.value;
    }
}

void UIManager::render(){
    glDisable( GL_CULL_FACE );
    glDisable(GL_DEPTH_TEST);

    uiProgram.updateUniforms();
    textures->bind(0);
    mainFont->getAtlas()->bind(1);
    drawBuffer->draw();
}

void UIManager::mouseMove(int x, int y){
    mousePosition.x = x;
    mousePosition.y = y;

    UIFrame* element = getElementUnder(x,y);
    underScrollHover = getElementUnder(x,y,true);

    if(element != underHover && underHover != nullptr){
        underHover->setHover(false);   
        if(underHover->onMouseLeave) underHover->onMouseLeave(*this);
    }
    if(element != nullptr){
        element->setHover(true);
        if(element->onMouseEnter) element->onMouseEnter(*this);
    }
    underHover = element;

    if(underHover && underHover->onMouseMove) underHover->onMouseMove(*this,x,y);
    if(inFocus && inFocus != underHover && inFocus->onMouseMove) inFocus->onMouseMove(*this,x,y);

    update();
}

void UIManager::mouseEvent(GLFWwindow* window, int button, int action, int mods){
    if(button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS){
        if(inFocus != underHover){
            if(inFocus) inFocus->setFocus(false);
            inFocus = underHover;
            if(inFocus) inFocus->setFocus(true);
        }
        if(underHover && underHover->onClicked) underHover->onClicked();
    }

    if(underHover && underHover->onMouseEvent) underHover->onMouseEvent(*this,button,action,mods);
    if(inFocus && inFocus != underHover && inFocus->onMouseEvent) inFocus->onMouseEvent(*this,button,action,mods);

    update();
}

void UIManager::scrollEvent(GLFWwindow* window, double xoffset, double yoffset){
    if(!underScrollHover) return;
    if(underScrollHover->onScroll) underScrollHover->onScroll(*this,xoffset,yoffset);
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
    if(underHover){
        underHover->setHover(false);
        //if(underHover->onMouseLeave) underHover->onMouseLeave(*this);
    }
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

UIFrame* UIManager::getElementUnder(int x, int y, bool onlyScrollable){
    if(currentWindow == (UIWindowIdentifier)-1) return nullptr;

    std::queue<std::tuple<int, UIFrame*, UIFrame*>> elements;
    
    for(auto& window: getCurrentWindow().getCurrentLayer().getElements()) elements.push({0,window.get(), nullptr});
    
    UIFrame* current = nullptr;
    int cdepth = -1;

    while(!elements.empty()){
        auto [depth,element,parent] = elements.front();
        elements.pop();

        if(!element->pointWithin({x,y})) continue;

        for(auto& child: element->children) elements.push({depth + 1, child.get(), element});

        if(onlyScrollable && !element->isScrollable()) continue;
        if(!element->isHoverable()) continue;
        if(cdepth >= depth) continue;
        
        current = element;
    }

    return current;
}

bool UIFrame::pointWithinBounds(glm::vec2 point, UITransform t, int padding){
    return  point.x > t.x - padding && point.x < t.x + t.width  + padding &&
            point.y > t.y - padding && point.y < t.y + t.height + padding;
}

bool UIFrame::pointWithin(glm::vec2 point, int padding){
    return pointWithinBounds(point, transform, padding);
}

bool UIFrame::updateRenderData(){
    auto newRenderData = getRenderingInformation();
    
    for(int i = 0; i < newRenderData.size();i++){
        auto renderInfo = newRenderData[i].process();

        if(i >= renderDataLocations.size()) 
            renderDataLocations.push_back(manager.appendRenderInfo(renderInfo));
        else manager.updateRenderInfo(renderDataLocations[i], renderInfo);
    }

    if(renderDataLocations.size() > newRenderData.size()) renderDataLocations.resize(newRenderData.size());

    return true;
}
void UIFrame::clearRenderData(){
    manager.clearRenderInfo(renderDataLocations);
    renderDataLocations.clear();
}
void UIFrame::updateChildRenderData(){
    updateRenderData();
    for(auto& child: children) child->updateRenderData();
}

std::vector<UIRenderInfo> UIFrame::getRenderingInformation(){
    std::vector<UIRenderInfo> out = {
        UIRenderInfo::Rectangle(
            transform.x,transform.y,transform.width,transform.height,
            getAttribute(&Style::backgroundColor),
            borderSizes,
            getAttribute(&Style::borderColor))
    };

    return out;
};

void UIFrame::calculateTransforms(){
    auto margin = getAttribute(&UIFrame::Style::margin);

    auto borderWidth = getAttribute(&Style::borderWidth);
    borderSizes = {
        getValueInPixels(borderWidth[0], false),
        getValueInPixels(borderWidth[1], true ),
        getValueInPixels(borderWidth[2], false),
        getValueInPixels(borderWidth[3], true )
    };

    int margin_x = getValueInPixels(margin, true );
    int margin_y = getValueInPixels(margin, false);

    int px      = getValueInPixels(x     , true );
    int py      = getValueInPixels(y     , false);
    int pwidth  = getValueInPixels(width , true );
    int pheight = getValueInPixels(height, false);
    
    int offset_x = 0, offset_y = 0;
    if(parent){
        auto t = parent->contentTransform;
        offset_x = t.x;
        offset_y = t.y;
    }

    boundingTransform = {
        px + offset_x,
        py + offset_y,
        pwidth  + borderSizes.right  + borderSizes.left + margin_x * 2,
        pheight + borderSizes.bottom + borderSizes.top  + margin_y * 2
    };

    transform = {
        px + offset_x + margin_x,
        py + offset_y + margin_y,
        pwidth  + borderSizes.right  + borderSizes.left,
        pheight + borderSizes.bottom + borderSizes.top
    };

    contentTransform = {
        px + offset_x + margin_x + borderSizes.left,
        px + offset_y + margin_y + borderSizes.top,
        pwidth,
        pheight
    };

    clipRegion = transform.asRegion();

    if(parent){
        clipRegion.min.x = glm::clamp(clipRegion.min.x, parent->clipRegion.min.x, parent->clipRegion.max.x);
        clipRegion.min.y = glm::clamp(clipRegion.min.y, parent->clipRegion.min.y, parent->clipRegion.max.y);
        clipRegion.max.x = glm::clamp(clipRegion.max.x, parent->clipRegion.min.x, parent->clipRegion.max.x);
        clipRegion.max.y = glm::clamp(clipRegion.max.y, parent->clipRegion.min.y, parent->clipRegion.max.y);
    }
}

std::vector<UIRenderInfo> UILabel::getRenderingInformation() {
    auto t = getTextPosition(manager);
    glm::vec2 textDimensions = manager.getMainFont().getTextDimensions(text);

    //std::cout << "Label with text: " << text << " has width of unit: " << width.unit << " and height of unit: " << height.unit << 
    //"That is" << width.value << " and " << height.value << std::endl;

    //std::cout << textDimensions.x << " " << textDimensions.y << std::endl;

    if(width .unit == NONE) width  = textDimensions.x + textPadding * 2;
    if(height.unit == NONE) height = textDimensions.y + textPadding * 2;

    std::vector<UIRenderInfo> out = UIFrame::getRenderingInformation();
    std::vector<UIRenderInfo> temp = manager.buildTextRenderingInformation(text,t.x,t.y,1,getAttribute(&Style::textColor));

    for(auto& i: temp){
        i.clip = true;
        i.clipRegion = clipRegion;
        out.push_back(i);
    }

    return out;
}

UITransform UILabel::getTextPosition(UIManager& manager){
    glm::vec2 textDimensions = manager.getMainFont().getTextDimensions(text);

    auto textPosition = getAttribute(&Style::textPosition);

    int tx = 0;
    if     (textPosition == UIFrame::Style::TextPosition::LEFT  ) tx = transform.x + textPadding;
    else if(textPosition == UIFrame::Style::TextPosition::CENTER) tx = transform.x + transform.width  / 2 - textDimensions.x / 2;
    else if(textPosition == UIFrame::Style::TextPosition::RIGHT ) tx = transform.x + transform.width - textDimensions.x - textPadding;

    int ty = transform.y + transform.height / 2 - textDimensions.y / 2;

    return {
        tx,
        ty
    };
}

std::vector<UIRenderInfo> UIImage::getRenderingInformation(){
    if(!loaded){
        manager.getTextures()->addTexture(path);
        loaded = true;
    }
    
    std::vector<UIRenderInfo> out = {UIRenderInfo::Texture(
        transform.x,transform.y,transform.width,transform.height,
        manager.getTextures()->getTextureUVs(path),
        manager.getTextures()->getTextureIndex(path)
    )};

    return out;
}

UIInput::UIInput(UIManager& manager): UILabel(manager) {
    this->focusable = true;

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

std::vector<UIRenderInfo> UIInput::getRenderingInformation(){
    glm::vec2 textDimensions = manager.getMainFont().getTextDimensions(text);

    auto tpos = getTextPosition(manager);
    auto textColor = getAttribute(&UIFrame::Style::textColor);
    
    std::vector<UIRenderInfo> out = UILabel::getRenderingInformation();

    if(focus){
        out.push_back(UIRenderInfo::Rectangle(tpos.x + textDimensions.x,transform.y + transform.height / 6,3,(transform.height / 3) * 2, textColor));    
    }

    //out.insert(out.end(), temp.begin(), temp.end());

    return out;
}

UISlider::UISlider(UIManager& manager): UIFrame(manager) {
    this->focusable = true;
    
    onMouseEvent = [this](UIManager& manager, int button, int action, int mods){
        if(!this->hover) return;
        if(button != GLFW_MOUSE_BUTTON_1) return;

        auto t = this->getHandleTransform(manager);
        if(action == GLFW_RELEASE){
            grabbed = false;
        }
        else if(pointWithinBounds(manager.getMousePosition(), t, 5) && action == GLFW_PRESS){
            grabbed = true;
        }
        else if(action == GLFW_PRESS){
            moveTo(manager,manager.getMousePosition());
        }
    };
    
    onMouseMove = [this](UIManager& manager, int x, int y){
        if(!grabbed) return;
    
        moveTo(manager,{x,y});
    };

    //onMouseLeave = [this](UIManager& manager){
    //    grabbed = false;
    //};
}

UITransform UISlider::getHandleTransform(UIManager& manager){
    int range = this->max - this->min;

    float percentage = 0.0f;
    if(range != 0) percentage = static_cast<float>(*this->value - this->min) / static_cast<float>(range);

    int handlePos = static_cast<float>(orientation == HORIZONTAL ? transform.width : transform.height) * percentage;
    //std::cout << handlePos << " " << percentage << std::endl;

    int handleWidthI = static_cast<int>(handleWidth);
    handlePos = handlePos - handleWidthI / 2;

    return{
        transform.x + (orientation == HORIZONTAL ? handlePos : 0),
        transform.y + (orientation == VERTICAL   ? handlePos : 0),
        (orientation == HORIZONTAL) ? handleWidthI : transform.width ,
        (orientation == VERTICAL  ) ? handleWidthI : transform.height
    };
}

void  UISlider::moveTo(UIManager& manager, glm::vec2 pos){
    float percentage = 0.0f;

    if     (orientation == HORIZONTAL) percentage = static_cast<float>(pos.x - transform.x) / static_cast<float>(transform.width );
    else if(orientation == VERTICAL  ) percentage = static_cast<float>(pos.y - transform.y) / static_cast<float>(transform.height);

    percentage = glm::clamp(percentage, 0.0f, 1.0f);

    *this->value = (this->max - this->min) * percentage + this->min;
}

std::vector<UIRenderInfo> UISlider::getRenderingInformation(){
    std::vector<UIRenderInfo> out = {
        UIRenderInfo::Rectangle(
            transform,
            getAttribute(&Style::backgroundColor),
            borderSizes,
            getAttribute(&Style::borderColor)
        )
    };

    //auto ht = getHandleTransform(manager);
    //std::cout << ht.x << " " << ht.y << " " << ht.width << " " << ht.height << std::endl;

    out.push_back(UIRenderInfo::Rectangle(getHandleTransform(manager), handleColor, {3,3,3,3},UIRenderInfo::generateBorderColors(handleColor)));
    
    std::string text = std::to_string(*value);
    glm::vec2 textDimensions = manager.getMainFont().getTextDimensions(text);

    if(displayValue){
        int tx = transform.x + transform.width + valueDisplayOffset;
        int ty = transform.y + transform.height / 2 - textDimensions.y / 2;

        std::vector<UIRenderInfo> temp = manager.buildTextRenderingInformation(text,tx,ty,1,{1,1,1,1});
        out.insert(out.end(), temp.begin(), temp.end());
    }

    return out;
}

void UIFlexFrame::calculateTransforms(){
    UIFrame::calculateTransforms();

    int size = 0;
        
    for(auto& child: children){
        auto ct = child->getBoundingTransform();

        size += getValueInPixels(elementMargin, direction == COLUMN);
        size += direction == COLUMN ? ct.width : ct.height;
    } 
    if(direction == COLUMN) width = {size};
    else height = {size};

    int offset = 0;
    for(auto& child: children){
        offset += getValueInPixels(elementMargin, direction == COLUMN);

        auto ct = child->getBoundingTransform();

        child->setPosition(
            direction == COLUMN ? TValue(PIXELS,offset) : transform.width  / 2 - ct.width  / 2,
            direction == ROWS   ? TValue(PIXELS,offset) : transform.height / 2 - ct.height / 2
        );
        offset += 
            direction == COLUMN ?
            ct.width :
            ct.height;
    }
}

UIScrollableFrame::UIScrollableFrame(UIManager& manager): UIFrame(manager) {
    this->body = std::make_shared<UIFlexFrame>();
    body->setExpand(true);
    body->setSize({PERCENT,100},0);
    scrollable = true;

    onScroll = [this](UIManager& manager, double xoffset, double yoffset){
        this->scroll += yoffset * 30 * -1;
        this->scroll = glm::clamp(this->scroll, 0, this->scrollMax);
    };

    slider = std::make_shared<UISlider>(0,0,0,0, &scroll, 0, scrollMax);
    slider->setOrientation(UISlider::VERTICAL);
    slider->setDisplayValue(false);
    slider->setHandleWidth(60);
    
    UIFrame::appendChild(this->body);
    UIFrame::appendChild(slider);
}

void UIScrollableFrame::calculateTransforms(){
    auto bodyT = body->getBoundingTransform();

    //std::cout << bodyT.height << std::endl;
    scrollMax = std::max(bodyT.height - contentTransform.height, 0);
    body->setPosition(0,-scroll);
    //body->setSize(bodyT.width - sliderWidth, t.height);
    
    slider->setPosition(contentTransform.width - sliderWidth,0);
    slider->setSize(sliderWidth, contentTransform.height);
    slider->setMax(scrollMax);
}