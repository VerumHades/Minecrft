#include <ui/layouts.hpp>

UITransform UILayout::calculateContentTransform(UIFrame* frame){
    return frame->getViewportTransform();
}

void UILayout::arrangeChildren(UIFrame* frame){
    int offsetX = 0;
    int offsetY = 0;
    int greatestY = 0;

    auto& frame_content_transform = frame->getContentTransform();
    for(auto& child: frame->getChildren()){
        auto& bounding_transform = child->getBoundingTransform();
        
        greatestY = std::max(child->getBoundingTransform().height,greatestY);

        if(offsetX + bounding_transform.width > frame_content_transform.width){
            offsetX = 0;
            offsetY += greatestY;
            greatestY = 0;

            child->setPosition(TValue::Pixels(offsetX),TValue::Pixels(offsetY));
            continue;
        }

        child->setPosition(TValue::Pixels(offsetX),TValue::Pixels(offsetY));
        
        offsetX += child->getBoundingTransform().width;
    }
}

UITransform UIFlexLayout::calculateContentTransform(UIFrame* frame){
    auto& viewport_transform = frame->getViewportTransform();
    int size = 0;
        
    for(auto& child: frame->getChildren()){
        child->calculateElementsTransforms();
        auto ct = child->getBoundingTransform();

        size += direction == HORIZONTAL ? ct.width : ct.height;
    } 

    return {
        viewport_transform.x,
        viewport_transform.y,
        direction == HORIZONTAL ? size : viewport_transform.width,
        direction == VERTICAL   ? size : viewport_transform.height
    };
}
void UIFlexLayout::arrangeChildren(UIFrame* frame) {
    int offset = 0;

    auto& content_transform = frame->getContentTransform();

    if(fill){
        int size = (direction == HORIZONTAL ? content_transform.width : content_transform.height) / frame->getChildren().size();

        for(auto& child: frame->getChildren()){
            child->setPosition(
                TValue::Pixels(direction == HORIZONTAL ? offset : 0),
                TValue::Pixels(direction == VERTICAL   ? offset : 0)
            );
            child->setSize(
                TValue::Pixels(direction == HORIZONTAL ? size : content_transform.width),
                TValue::Pixels(direction == VERTICAL   ? size : content_transform.height)
            );

            offset += size;
        }

        return;
    }

    for(auto& child: frame->getChildren()){
        
        child->calculateElementsTransforms();
        auto ct = child->getBoundingTransform();

        child->setPosition(
            TValue::Pixels(direction == HORIZONTAL ? offset : static_cast<float>(content_transform.width ) / 2.0f - static_cast<float>(ct.width ) / 2.0f),
            TValue::Pixels(direction == VERTICAL   ? offset : static_cast<float>(content_transform.height) / 2.0f - static_cast<float>(ct.height) / 2.0f)
        );
        offset += 
            direction == HORIZONTAL ?
            ct.width :
            ct.height;
    }
}