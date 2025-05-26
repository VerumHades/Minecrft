#pragma once

#include <ui/color.hpp>

class UIFrame;

/**
 * @brief Base class for all layouts
 * 
 */
class UILayout{
    public:
        /**
        * @brief Resizes or changes the element itself
        * 
        * @param frame 
        * @return UITransform 
        */
        virtual UITransform calculateContentTransform(UIFrame* frame);
        /**
         * @brief Organizes all of the elements children in some defined way
         * 
         * @param frame 
         */
        virtual void arrangeChildren(UIFrame* frame);
};

/**
 * @brief A flex layout akin to the css counterpart used on the web
 * 
 */
class UIFlexLayout: public UILayout{
    public:
        enum FlexDirection{
            HORIZONTAL,
            VERTICAL
        };

    private:
        FlexDirection direction = VERTICAL;
        bool expandToChildren = false;
        bool fill = false;

    public:
        UIFlexLayout(FlexDirection direction = VERTICAL, bool expand = false): direction(direction), expandToChildren(expand) {};
        void setExpand(bool value) {expandToChildren = value;}
        void setFill(bool value) {fill = value;}
        void setDirection(FlexDirection direction) {this->direction = direction;}
        
        UITransform calculateContentTransform(UIFrame* frame) override;
        void arrangeChildren(UIFrame* frame) override;
};

#include <ui/core.hpp>