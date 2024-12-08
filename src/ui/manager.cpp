#include <ui/manager.hpp>

UIManager::UIManager(UIBackend* backend): loader(*this), backend(backend){

}

void UIManager::resize(int width, int height){
    screenWidth = width;
    screenHeight = height;

    backend->resizeVieport(width,height);

    if(!getCurrentWindow()) return;

    updateAll();
}

void UIManager::updateAll(){
    auto* window = getCurrentWindow();
    if(!window) return;

    for(auto& element: window->getCurrentLayer().getElements()){
        element->calculateTransforms();
        element->update();
        element->updateChildren();
    }
}
void UIManager::stopDrawingAll(){
    auto* window = getCurrentWindow();
    if(!window) return;

    for(auto& element: window->getCurrentLayer().getElements()){
        element->stopDrawing();
        element->stopDrawingChildren();
    }
}

void UIFrame::update(){
    UIRenderBatch batch;

    getRenderingInformation(batch);
    
    batch.texture = dedicated_texture_array.get();        
    batch.clipRegion = clipRegion;

    stopDrawing();
    draw_batch_iterator = manager.getBackend()->addRenderBatch(batch);
    has_draw_batch = true;

   // std::cout << "Update element hover: " << this->identifiers.tag << " #" << this->identifiers.id << " ";
    //for(auto& classname: this->identifiers.classes) std::cout << "." << classname;
    //std::cout << " Element has state: " << state << std::endl;
    //std::cout << "Render members in total: " << accumulatedRenderInfo.size() << std::endl;
}

void UIFrame::stopDrawing(){
    if(!has_draw_batch) return;

    manager.getBackend()->removeBatch(draw_batch_iterator);
    has_draw_batch = false;
}

void UIFrame::updateChildren(){
    for(auto& child: children){
        child->update();
        child->updateChildren();
    }
}

void UIFrame::stopDrawingChildren(){
    for(auto& child: children){
        child->stopDrawing();
        child->stopDrawingChildren();
    }
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

        case FIT_CONTENT: return horizontal ? contentTransform.width : contentTransform.height;

        case MY_PERCENT: 
            return static_cast<float>(
                horizontal ? 
                getValueInPixels(width,  horizontal) : 
                getValueInPixels(height, horizontal)
            ) / 100.0f * value.value;

        case PERCENT:
            if(parent){
                auto t = parent->viewportTransform;
                return static_cast<float>(
                    horizontal ? 
                    t.width  - borderSizes.left - borderSizes.right  - margin_x * 2: 
                    t.height - borderSizes.top  - borderSizes.bottom - margin_y * 2
                ) / 100.0f * value.value;
            }
            else return (( horizontal ? manager.getScreenWidth() : manager.getScreenHeight() )  / 100.0f) * value.value;
        default:
            std::cerr << "Invalid TValue?" << std::endl;
            return 0;
    }
}

void UIManager::renderElementAndChildren(std::shared_ptr<UIFrame>& element){
    if(element->has_draw_batch){
        backend->renderBatch(element->draw_batch_iterator);
    }
    
    for(auto& child: element->children){
        renderElementAndChildren(child);
    }
}

void UIManager::render(){
    auto* window = getCurrentWindow();
    if(!window) return;

    backend->setupRender();

    for(auto& element: window->getCurrentLayer().getElements()){
        renderElementAndChildren(element);
    }

    backend->cleanupRender();
}

void UIManager::mouseMove(int x, int y){
    mousePosition.x = x;
    mousePosition.y = y;

    auto element = getElementUnder(x,y);
    underScrollHover = getElementUnder(x,y,true);

    if(element == underHover) return;

    if(element != underHover && underHover != nullptr){
        underHover->setHover(false);
        underHover->update(); 
        if(underHover->onMouseLeave) underHover->onMouseLeave(*this);
    }

    if(element != nullptr){
        element->setHover(true);
        element->update();

        //std::cout << "Newly under hover: " << element->identifiers.tag << " #" << element->identifiers.id << " ";
        //for(auto& classname: element->identifiers.classes) std::cout << "." << classname;
        //std::cout << " Element has state: " << element->state << std::endl;

        if(element->onMouseEnter) element->onMouseEnter(*this);
    }

    underHover = element;

    if(underHover && underHover->onMouseMove) underHover->onMouseMove(*this,x,y);
    if(inFocus && inFocus != underHover && inFocus->onMouseMove) inFocus->onMouseMove(*this,x,y);
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

    if(underHover) underHover->update();
    if(inFocus) inFocus->update();
}

void UIManager::scrollEvent(GLFWwindow* window, double xoffset, double yoffset){
    if(!underScrollHover) return;
    if(underScrollHover->onScroll) underScrollHover->onScroll(*this,xoffset,yoffset);
    
    if(underScrollHover){
        underScrollHover->update();
        underScrollHover->updateChildren();
    }
}

void UIManager::keyTypedEvent(GLFWwindow* window, unsigned int codepoint){
    if(!inFocus) return;
    if(inFocus->onKeyTyped) inFocus->onKeyTyped(window,codepoint);
    
    if(inFocus) inFocus->update();
}
void UIManager::keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(!inFocus) return;
    if(inFocus->onKeyEvent) inFocus->onKeyEvent(window,key,scancode,action,mods);

    if(inFocus) inFocus->update();
}


void UIManager::resetStates(){
    if(underHover){
        underHover->setHover(false);
        //if(underHover->onMouseLeave) underHover->onMouseLeave(*this);
    }
    if(inFocus){
        inFocus->setFocus(false);
    }
    inFocus = nullptr;
    underHover = nullptr;
    underScrollHover = nullptr;
}

void UIManager::setCurrentWindow(UIWindowIdentifier id){
    resetStates();
    stopDrawingAll();
    currentWindow = id;
    
    if(!getCurrentWindow()) return;
    
    updateAll();
}
UIWindow* UIManager::getCurrentWindow(){
    if(currentWindow < 0 || currentWindow >= windows.size()) return nullptr;
    return &windows[currentWindow];
}
UIWindow* UIManager::getWindow(UIWindowIdentifier id){
    if(id < 0 || id >= windows.size()) return nullptr;
    return &windows[id];
}
UIWindowIdentifier UIManager::createWindow(){
    int id = windows.size();
    windows.push_back({});
    return id;
}

void UIManager::loadWindowFromXML(UIWindow& window, std::string load_path){
    loader.loadWindowFromXML(window, load_path);
}

void UIFrame::appendChild(std::shared_ptr<UIFrame> child){
    child->parent = this;
    child->zIndex = this->zIndex + 1;
    children.push_back(child);

    manager.getLoader().getCurrentStyle().applyTo(child);

    child->calculateTransforms();
}
void UIFrame::clearChildren(){
    children.clear();
}
std::shared_ptr<UIFrame> UIManager::getElementUnder(int x, int y, bool onlyScrollable){
    auto* current_window = getCurrentWindow();
    if(!current_window) return nullptr;

    std::queue<std::tuple<int, std::shared_ptr<UIFrame>, std::shared_ptr<UIFrame>>> elements;

    for(auto& window: current_window->getCurrentLayer().getElements()) elements.push({0,window, nullptr});
    
    std::shared_ptr<UIFrame> current = nullptr;
    int cdepth = -1;

    while(!elements.empty()){
        auto [depth,element,parent] = elements.front();
        elements.pop();

        if(!element->pointWithin({x,y})) continue;

        for(auto& child: element->children) elements.push({depth + 1, child, element});

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

void UIFrame::getRenderingInformation(UIRenderBatch& batch){
    auto bg = getAttribute(&Style::backgroundColor);
    if(bg == UIColor{0,0,0,0}) return;

    batch.BorderedRectangle(
        transform.x,transform.y,transform.width,transform.height,
        bg,
        borderSizes,
        getAttribute(&Style::borderColor)
    );
};

static inline void reduceRegionTo(UIRegion& target, UIRegion& to){
    target.min.x = glm::clamp(target.min.x, to.min.x, to.max.x);
    target.min.y = glm::clamp(target.min.y, to.min.y, to.max.y);
    target.max.x = glm::clamp(target.max.x, to.min.x, to.max.x);
    target.max.y = glm::clamp(target.max.y, to.min.y, to.max.y);
}

void UIFrame::calculateElementsTransforms(){
    auto margin = getAttribute(&UIFrame::Style::margin);
    auto borderWidth = getAttribute(&Style::borderWidth);

    font_size = getValueInPixels(getAttribute(&UIFrame::Style::fontSize), true);

    if(layout) contentTransform = layout->calculateContentTransform(this);

    borderSizes = {
        getValueInPixels(borderWidth[0], false),
        getValueInPixels(borderWidth[1], true ),
        getValueInPixels(borderWidth[2], false),
        getValueInPixels(borderWidth[3], true )
    };

    margin_x = getValueInPixels(margin, true );
    margin_y = getValueInPixels(margin, false);

    UITransform internalTransform = {
        getValueInPixels(x     , true ),
        getValueInPixels(y     , false),
        getValueInPixels(width , true ),
        getValueInPixels(height, false)
    };

    int offset_x = 0, offset_y = 0;
    
    //UIFrame* temp = this;
    //while(temp) {temp = parent->parent; std::cout << temp << "->";}
    if(parent){
        //std::cout << "From parent: " << contentTransform.to_string() << std::endl;
        offset_x = parent->contentTransform.x;
        offset_y = parent->contentTransform.y;

        zIndex = parent->zIndex + 1;
    }

    boundingTransform = {
        internalTransform.x + offset_x,
        internalTransform.y + offset_y,
        internalTransform.width  + borderSizes.right  + borderSizes.left + margin_x * 2,
        internalTransform.height + borderSizes.bottom + borderSizes.top  + margin_y * 2
    };

    transform = {
        internalTransform.x + offset_x + margin_x,
        internalTransform.y + offset_y + margin_y,
        internalTransform.width  + borderSizes.right  + borderSizes.left,
        internalTransform.height + borderSizes.bottom + borderSizes.top
    };

    viewportTransform = {
        internalTransform.x + offset_x + margin_x + borderSizes.left,
        internalTransform.y + offset_y + margin_y + borderSizes.top,
        internalTransform.width,
        internalTransform.height
    };

    if(layout) contentTransform = layout->calculateContentTransform(this);
    else contentTransform = viewportTransform;
    //std::cout << internalTransform.to_string() << std::endl;
    //std::cout << boundingTransform.to_string() << std::endl;
    //std::cout << transform.to_string() << std::endl;
    //std::cout << contentTransform.to_string() << std::endl;

    clipRegion        = transform.asRegion();
    contentClipRegion = viewportTransform.asRegion();

    if(parent){
        reduceRegionTo(clipRegion       , parent->contentClipRegion);
        reduceRegionTo(contentClipRegion, parent->contentClipRegion);
    }

    //clipRegion = {{0,0},{100,100}};
}

void UIFrame::calculateChildrenTransforms(){
    if(layout) layout->arrangeChildren(this);
    for(auto& child: children){
        child->calculateTransforms();
    }
}

void UIFrame::calculateTransforms(){
    calculateElementsTransforms();
    calculateChildrenTransforms();
}

void UILabel::getRenderingInformation(UIRenderBatch& batch) {
    auto t = getTextPosition(manager);
    UITextDimensions textDimensions = manager.getBackend()->getTextDimensions(text, font_size);

    //std::cout << "Label with text: " << text << " has width of unit: " << width.unit << " and height of unit: " << height.unit << 
    //"That is" << width.value << " and " << height.value << std::endl;

    //std::cout << textDimensions.x << " " << textDimensions.y << std::endl;

    if(width .unit == NONE) width  = textDimensions.width + textPadding * 2;
    if(height.unit == NONE) height = textDimensions.height + textPadding * 2;

    UIFrame::getRenderingInformation(batch);
    batch.Text(text, t.x, t.y, font_size, getAttribute(&Style::textColor));
}

UITransform UILabel::getTextPosition(UIManager& manager){
    UITextDimensions textDimensions = manager.getBackend()->getTextDimensions(text, font_size);

    auto textPosition = getAttribute(&Style::textPosition);

    int tx = 0;
    if     (textPosition == UIFrame::Style::TextPosition::LEFT  ) tx = contentTransform.x + textPadding;
    else if(textPosition == UIFrame::Style::TextPosition::CENTER) tx = contentTransform.x + contentTransform.width  / 2 - textDimensions.width / 2;
    else if(textPosition == UIFrame::Style::TextPosition::RIGHT ) tx = contentTransform.x + contentTransform.width - textDimensions.width - textPadding;

    int ty = contentTransform.y + contentTransform.height / 2 - textDimensions.height / 2;

    return {
        tx,
        ty
    };
}

UIInput::UIInput(UIManager& manager): UILabel(manager) {
    identifiers.tag = "input";

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

void UIInput::getRenderingInformation(UIRenderBatch& batch){
    UITextDimensions textDimensions = manager.getBackend()->getTextDimensions(text, font_size);

    auto tpos = getTextPosition(manager);
    auto textColor = getAttribute(&UIFrame::Style::textColor);
    
    UILabel::getRenderingInformation(batch);

    batch.Rectangle(
        {
            tpos.x + textDimensions.width,
            transform.y + transform.height / 6,
            3,
            (transform.height / 3) * 2
        },
        textColor
    );
    //out.insert(out.end(), temp.begin(), temp.end());
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
        update();
    };
    
    onMouseMove = [this](UIManager& manager, int x, int y){
        if(!grabbed) return;
    
        moveTo(manager,{x,y});
        update();
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

    if(onMove) onMove();
}

void UISlider::getRenderingInformation(UIRenderBatch& batch){
    batch.BorderedRectangle(
        transform,
        getAttribute(&Style::backgroundColor),
        borderSizes,
        getAttribute(&Style::borderColor)
    );

    //auto ht = getHandleTransform(manager);
    //std::cout << ht.x << " " << ht.y << " " << ht.width << " " << ht.height << std::endl;

    batch.Rectangle(getHandleTransform(manager), handleColor);
    
    std::string text = std::to_string(*value);
    UITextDimensions textDimensions = manager.getBackend()->getTextDimensions(text, font_size);

    if(displayValue){
        int tx = transform.x + valueDisplayOffset;
        int ty = transform.y + transform.height / 2 - textDimensions.height / 2;

        batch.Text(text,tx,ty,font_size,getAttribute(&Style::textColor));
    }
}

UIScrollableFrame::UIScrollableFrame(UIManager& manager): UIFrame(manager) {
    identifiers.tag = "scrollable";
    
    scrollable = true;

    onScroll = [this](UIManager& manager, double xoffset, double yoffset){
        this->scroll += yoffset * 30 * -1;
        this->scroll = glm::clamp(this->scroll, 0, this->scrollMax);

        calculateTransforms();
        update();
        updateChildren();
    };

    /*slider = std::make_shared<UISlider>(manager);
    slider->setOrientation(UISlider::VERTICAL);
    slider->setDisplayValue(false);
    slider->setHandleWidth(60);
    slider->setValuePointer(&this->scroll);
    slider->setMin(0);

    slider->onMove = [this]{
        calculateTransforms();
    };

    UIFrame::appendChild(slider);*/
}

void UIScrollableFrame::calculateElementsTransforms(){
    UIFrame::calculateElementsTransforms();
    //std::cout << bodyT.height << std::endl;
    scrollMax = std::max(contentTransform.height - viewportTransform.height, 0);
    //body->setSize(bodyT.width - sliderWidth, t.height);
    contentTransform.y -= scroll;

    /*slider->setPosition(viewportTransform.width - sliderWidth,0);
    slider->setSize(sliderWidth, viewportTransform.height);
    slider->setMax(scrollMax);*/
}