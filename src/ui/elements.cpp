
#include <ui/elements.hpp>

void UIFrame::update(){
    UIRenderBatch batch;

    getRenderingInformation(batch);
    
    auto background_image = getAttribute(&Style::backgroundImage);
    batch.texture = background_image.texture ? background_image.texture.get() : dedicated_texture_array.get();
    batch.clipRegion = clipRegion;
    
    stopDrawing();
    
    if(batch.commands.size() == 0) return;
    //std::cout << "Drawing: "  << std::endl;
    draw_batch_iterator = UICore::get().getBackend().addRenderBatch(batch);
    has_draw_batch = true;

   // std::cout << "Update element hover: " << this->identifiers.tag << " #" << this->identifiers.id << " ";
    //for(auto& classname: this->identifiers.classes) std::cout << "." << classname;
    //std::cout << " Element has state: " << state << std::endl;
    //std::cout << "Render members in total: " << accumulatedRenderInfo.size() << std::endl;
}

void UIFrame::stopDrawing(){
    if(!has_draw_batch) return;

    UICore::get().getBackend().removeBatch(draw_batch_iterator);
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
        case AUTO: return horizontal ? prefferedSize.width : prefferedSize.height;
        case PIXELS: return value.value;   

        case WINDOW_WIDTH : return (UICore::get().getScreenWidth()  / 100.0f) * value.value;
        case WINDOW_HEIGHT: return (UICore::get().getScreenHeight() / 100.0f) * value.value;
        
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
                    t.width  - borderSizes.left - borderSizes.right  - margin.horizontal() :
                    t.height - borderSizes.top  - borderSizes.bottom - margin.vertical()
                ) / 100.0f * value.value;
            }
            else return (( horizontal ? UICore::get().getScreenWidth() : UICore::get().getScreenHeight() )  / 100.0f) * value.value;
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
    auto background_image = getAttribute(&Style::backgroundImage);

    if(!background_image.texture){
        auto bg = getAttribute(&Style::backgroundColor);
        if(bg == UIColor{0,0,0,0}) return;

        batch.BorderedRectangle(
            transform,
            bg,
            borderSizes,
            getAttribute(&Style::borderColor)
        );
    }
    else batch.Texture(transform, {{0,0},{1,1}});
};

static inline void reduceRegionTo(UIRegion& target, UIRegion& to){
    target.min.x = glm::clamp(target.min.x, to.min.x, to.max.x);
    target.min.y = glm::clamp(target.min.y, to.min.y, to.max.y);
    target.max.x = glm::clamp(target.max.x, to.min.x, to.max.x);
    target.max.y = glm::clamp(target.max.y, to.min.y, to.max.y);
}

void UIFrame::calculateElementsTransforms(){
    auto margin_t = getAttribute(&UIFrame::Style::margin);
    auto padding_t = getAttribute(&UIFrame::Style::padding);
    auto translation_t = getAttribute(&UIFrame::Style::translation);
    auto borderWidth = getAttribute(&Style::borderWidth);

    font_size = getValueInPixels(getAttribute(&UIFrame::Style::fontSize), true);

    if(layout) contentTransform = layout->calculateContentTransform(this);

    borderSizes = {
        getValueInPixels(borderWidth.top   , true ),
        getValueInPixels(borderWidth.right , false),
        getValueInPixels(borderWidth.bottom, true ),
        getValueInPixels(borderWidth.left  , false)
    };

    margin = {
        getValueInPixels(margin_t.top   , true ),
        getValueInPixels(margin_t.right , false),
        getValueInPixels(margin_t.bottom, true ),
        getValueInPixels(margin_t.left  , false)
    };

    padding = {
        getValueInPixels(padding_t.top   , true ),
        getValueInPixels(padding_t.right , false),
        getValueInPixels(padding_t.bottom, true ),
        getValueInPixels(padding_t.left  , false)
    };

    translation = {
        getValueInPixels(translation_t[0], true),
        getValueInPixels(translation_t[1], false)
    };

    UITransform internalTransform = {
        getValueInPixels(x     , true ) + translation.x,
        getValueInPixels(y     , false) + translation.y,
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
        internalTransform.width  + borderSizes.horizontal() + margin.horizontal() + padding.horizontal(),
        internalTransform.height + borderSizes.vertical()   + margin.vertical()   + padding.vertical()
    };

    transform = {
        internalTransform.x + offset_x + margin.left,
        internalTransform.y + offset_y + margin.top,
        internalTransform.width  + borderSizes.horizontal() + padding.horizontal(),
        internalTransform.height + borderSizes.vertical()   + padding.vertical()
    };

    viewportTransform = {
        internalTransform.x + offset_x + padding.left + margin.left + borderSizes.left,
        internalTransform.y + offset_y + padding.top  + margin.top + borderSizes.top,
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
    UITextDimensions textDimensions = UICore::get().getBackend().getTextDimensions(text, font_size);

    prefferedSize = {
        textDimensions.width + textPadding * 2,
        textDimensions.height + textPadding * 2
    };

    UIFrame::calculateElementsTransforms();
}

UITransform UILabel::getTextPosition(){
    UITextDimensions textDimensions = UICore::get().getBackend().getTextDimensions(text, font_size);

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

    onKeyTyped = [this](unsigned int codepoint){
        if(this->text.size() >= max_length) return;
        
        char typedChar = static_cast<char>(codepoint);

        if (typedChar >= 32 && typedChar <= 126) { 
            if(inputValidation && !inputValidation(typedChar)) return;
            this->text += typedChar;
        }
    };

    onKeyEvent = [this](int key, int scancode, int action, int mods){
        if(key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS){
            this->text = this->text.substr(0, this->text.size() - 1);
        }

        if(key == GLFW_KEY_ENTER && action == GLFW_PRESS){
            if(this->onSubmit) this->onSubmit(this->text);
        }
    };
}

void UIInput::getRenderingInformation(UIRenderBatch& batch){
    UITextDimensions textDimensions = UICore::get().getBackend().getTextDimensions(text, font_size);

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
        else if(pointWithinBounds(UICore::get().getMousePosition(), t, 5) && action == GLFW_PRESS){
            grabbed = true;
        }
        else if(action == GLFW_PRESS){
            moveTo(UICore::get().getMousePosition());
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
    UIFrame::getRenderingInformation(batch);
    //auto ht = getHandleTransform(UICore::get());
    //std::cout << ht.x << " " << ht.y << " " << ht.width << " " << ht.height << std::endl;

    batch.Rectangle(getHandleTransform(), handleColor);
    
    std::string text = std::to_string(*value);
    UITextDimensions textDimensions = UICore::get().getBackend().getTextDimensions(text, font_size);

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

    /*slider = std::make_shared<UISlider>(UICore::get());
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

    UICore::get().loader().getCurrentStyle().applyTo(child);

    child->calculateTransforms();

        //UICore::get().loader().getCurrentStyle().applyTo(child);
}
void UIFrame::clearChildren(){
    children.clear();
}

UIImage::UIImage(std::string path){
    dedicated_texture_array = UICore::LoadImage(path);
    if(!dedicated_texture_array) std::cerr << "Failed to load image '" << path << "'" << std::endl;
}

UIImage::UIImage(const Image& image){
    dedicated_texture_array = UICore::LoadImage(image);
    if(!dedicated_texture_array) std::cerr << "Failed to load image." << std::endl;
}

void UIImage::getRenderingInformation(UIRenderBatch& batch){
    batch.Texture(transform, {{0,0},{1,1}});
}