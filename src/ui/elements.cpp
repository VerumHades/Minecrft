
#include <ui/elements.hpp>

void UIFrame::update(){
    UIRenderBatch batch;

    getRenderingInformation(batch);
    
    batch.texture = dedicated_texture_array.get();        
    batch.clipRegion = clipRegion;
    
    stopDrawing();
    
    if(batch.commands.size() == 0) return;
    //std::cout << "Drawing: "  << std::endl;
    draw_batch_iterator = ui_core.getBackend().addRenderBatch(batch);
    has_draw_batch = true;

   // std::cout << "Update element hover: " << this->identifiers.tag << " #" << this->identifiers.id << " ";
    //for(auto& classname: this->identifiers.classes) std::cout << "." << classname;
    //std::cout << " Element has state: " << state << std::endl;
    //std::cout << "Render members in total: " << accumulatedRenderInfo.size() << std::endl;
}

void UIFrame::stopDrawing(){
    if(!has_draw_batch) return;

    ui_core.getBackend().removeBatch(draw_batch_iterator);
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
        case AUTO:{
            std::cout << prefferedSize.width << "x" << prefferedSize.height << std::endl;
            return horizontal ? prefferedSize.width : prefferedSize.height;
        }

        case WINDOW_WIDTH : return (ui_core.getScreenWidth()  / 100.0f) * value.value;
        case WINDOW_HEIGHT: return (ui_core.getScreenHeight() / 100.0f) * value.value;
        
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
            else return (( horizontal ? ui_core.getScreenWidth() : ui_core.getScreenHeight() )  / 100.0f) * value.value;
        default:
            std::cerr << "Invalid TValue?" << std::endl;
            return 0;
    }
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
    auto t = getTextPosition();

    UIFrame::getRenderingInformation(batch);
    batch.Text(text, t.x, t.y, font_size, getAttribute(&Style::textColor));
}

void UILabel::calculateElementsTransforms(){
    UIFrame::calculateElementsTransforms();

    UITextDimensions textDimensions = ui_core.getBackend().getTextDimensions(text, font_size);

    prefferedSize = {
        textDimensions.width + textPadding * 2,
        textDimensions.height + textPadding * 2
    };
}

UITransform UILabel::getTextPosition(){
    UITextDimensions textDimensions = ui_core.getBackend().getTextDimensions(text, font_size);

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

UIInput::UIInput(){
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
    UITextDimensions textDimensions = ui_core.getBackend().getTextDimensions(text, font_size);

    auto tpos = getTextPosition();
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

UISlider::UISlider() {
    this->focusable = true;
    
    onMouseEvent = [this](int button, int action, int mods){
        if(!this->hover) return;
        if(button != GLFW_MOUSE_BUTTON_1) return;

        auto t = this->getHandleTransform();
        if(action == GLFW_RELEASE){
            grabbed = false;
        }
        else if(pointWithinBounds(ui_core.getMousePosition(), t, 5) && action == GLFW_PRESS){
            grabbed = true;
        }
        else if(action == GLFW_PRESS){
            moveTo(ui_core.getMousePosition());
        }
        update();
    };
    
    onMouseMove = [this](int x, int y){
        if(!grabbed) return;
    
        moveTo({x,y});
        update();
    };

    //onMouseLeave = [this](){
    //    grabbed = false;
    //};
}

UITransform UISlider::getHandleTransform(){
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

void  UISlider::moveTo(glm::vec2 pos){
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

    //auto ht = getHandleTransform(ui_core);
    //std::cout << ht.x << " " << ht.y << " " << ht.width << " " << ht.height << std::endl;

    batch.Rectangle(getHandleTransform(), handleColor);
    
    std::string text = std::to_string(*value);
    UITextDimensions textDimensions = ui_core.getBackend().getTextDimensions(text, font_size);

    if(displayValue){
        int tx = transform.x + valueDisplayOffset;
        int ty = transform.y + transform.height / 2 - textDimensions.height / 2;

        batch.Text(text,tx,ty,font_size,getAttribute(&Style::textColor));
    }
}

UIScrollableFrame::UIScrollableFrame(){
    identifiers.tag = "scrollable";
    
    scrollable = true;

    onScroll = [this](double xoffset, double yoffset){
        this->scroll += yoffset * 30 * -1;
        this->scroll = glm::clamp(this->scroll, 0, this->scrollMax);

        calculateTransforms();
        update();
        updateChildren();
    };

    /*slider = std::make_shared<UISlider>(ui_core);
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

void UIFrame::appendChild(std::shared_ptr<UIFrame> child){
    child->parent = this;
    child->zIndex = this->zIndex + 1;
    children.push_back(child);

    ui_core.getLoader().getCurrentStyle().applyTo(child);

    child->calculateTransforms();
}
void UIFrame::clearChildren(){
    children.clear();
}