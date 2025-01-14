#pragma once

#include <ui/color.hpp>

class UIFrame;

/*
    Base class for all layouts
*/
class UILayout{
    public:
        /*
            Resizes or changes the element itself
        */
        virtual UITransform calculateContentTransform(UIFrame* frame);
        /*
            Organizes all of the elements children in some defined way
        */
        virtual void arrangeChildren(UIFrame* frame);
};

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