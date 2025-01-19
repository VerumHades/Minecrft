#pragma once

#include <memory>
#include <ui/font.hpp>
#include <queue>
#include <functional>
#include <optional>
#include <unordered_set>

#include <ui/color.hpp>
#include <ui/tvalue.hpp>
#include <ui/layouts.hpp>
#include <ui/backend.hpp>
#include <ui/core.hpp>



template <typename T>
class UIEventHandler{
    private:
        std::function<T> function;
        std::string lua_source = "";
    public:
        template <typename... Args>
        void trigger(Args&&... args) {
            if (function) {
                function(std::forward<Args>(args)...);
            }
            else if(lua_source != "") ui_core.lua().run(lua_source);
        }

        UIEventHandler& operator=(const std::string& other) {
            lua_source = other; // Set the function to the new function
            return *this;
        }

        UIEventHandler& operator=(const std::function<T>& other) {
            function = other; // Set the function to the new function
            return *this;
        }

        // Move assignment operator
        UIEventHandler& operator=(std::function<T>&& other) {
            function = std::move(other); // Move the function to the current object
            return *this;
        }
};


/*
    Core element that every other element inherits from
*/
class UIFrame{
    public:
        struct Style{
            enum TextPosition{
                LEFT,
                RIGHT,
                CENTER
            };

            std::optional<TextPosition>         textPosition;
            std::optional<UIColor>              textColor;
            std::optional<UIColor>              backgroundColor;
            std::optional<UISideSizesT>         borderWidth;
            std::optional<UIBorderColors>       borderColor;
            std::optional<UISideSizesT>         margin;
            std::optional<UISideSizesT>         padding;
            std::optional<TValue>               fontSize;
            std::optional<std::array<TValue,2>> translation;
        };

    protected:
        Style baseStyle = {
            Style::TextPosition::LEFT,
            UIColor{255,255,255,255},
            UIColor{0,0,0},
            UISideSizesT{0_px,0_px,0_px,0_px},
            UIBorderColors{UIColor{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
            UISideSizesT{0_px,0_px,0_px,0_px},
            UISideSizesT{0_px,0_px,0_px,0_px},
            24_px,
            std::array<TValue,2>{0_px,0_px}
        };
        Style hoverStyle;
        Style focusStyle;

        struct Identifiers{
            std::string tag = "frame";
            std::unordered_set<std::string> classes = {};
            std::string id = "";
        } identifiers;

        std::shared_ptr<UILayout> layout;

        Style& getStyleForState(UIElementState state){
            switch(state){
                case BASE : return baseStyle;
                case HOVER: return hoverStyle;
                case FOCUS: return focusStyle;
            }
            return baseStyle;
        }

        UIElementState state = BASE;

        TValue x = TNONE;
        TValue y = TNONE;
        TValue width  = {AUTO,0};
        TValue height = {AUTO,0};

        int zIndex = 0;
        
        bool hover = false;
        bool focus = false;
        bool focusable = false;
        bool hoverable = true;
        bool scrollable = false;

        std::vector<std::shared_ptr<UIFrame>> children;
        UIFrame* parent = nullptr;
        
        void setHover(bool value) {
            hover = value;

            if(state == FOCUS) return;
            if(hover) state = HOVER;
            else state = BASE;
        }
        void setFocus(bool value){
            focus = value;
            if(focus) state = FOCUS;
            else state = BASE;
        }
        bool pointWithin(glm::vec2 position, int padding = 0);
        bool pointWithinBounds(glm::vec2 position, UITransform transform, int padding = 0);

        int getValueInPixels(TValue value, bool horizontal);
        
        UITransform transform         = {0,0,0,0}; // Transform that includes the border
        UITransform viewportTransform = {0,0,0,0}; // Transform for only visible content
        UITransform contentTransform  = {0,0,0,0}; // Whole tranform for content
        UITransform boundingTransform = {0,0,0,0}; // Transform that includes margin
        UISideSizes borderSizes       = {0,0,0,0};
        UIRegion clipRegion           = {{0,0},{0,0}};
        UIRegion contentClipRegion    = {{0,0},{0,0}};

        UISize prefferedSize = {100,40};
        UITranslation translation = {0,0};
        UISideSizes margin  = 0;
        UISideSizes padding = 0;

        int font_size = 0;

        virtual void getRenderingInformation(UIRenderBatch& batch);

        std::shared_ptr<GLTextureArray> dedicated_texture_array;

        friend class UICore;
        friend class UILoader;
        friend class UIStyle;

        bool has_draw_batch = false;
        std::list<UIBackend::Batch>::iterator draw_batch_iterator;

    public:
        UIFrame() {
            identifiers.tag = "frame";
            //layout = std::make_unique<UILayout>();
        }

        ~UIFrame(){
            stopDrawingChildren();
            stopDrawing();
        }
        /*
            Event lambdas
        */
        UIEventHandler<void(int, int, int)> onMouseEvent;
        UIEventHandler<void(int, int)> onMouseMove;
        UIEventHandler<void(unsigned int)> onKeyTyped;
        UIEventHandler<void(int key, int scancode, int action, int mods)> onKeyEvent;
        UIEventHandler<void(void)> onClicked;

        UIEventHandler<void()> onMouseLeave;
        UIEventHandler<void()> onMouseEnter;

        UIEventHandler<void(int offsetX, int offsetY)> onScroll;

        void setPosition(TValue x, TValue y){this->x = x; this->y = y;}
        void setX(TValue x) {this->x = x;}
        void setY(TValue y) {this->y = y;}

        void setSize(TValue width, TValue height) {this->width = width; this->height = height;}
        void setWidth(TValue width) {this->width = width;}
        void setHeight(TValue height) {this->height = height;}

        void setIdentifiers(std::unordered_set<std::string> classes = {}, std::string id = "") {identifiers = {identifiers.tag,classes,id};}
        Identifiers& getIdentifiers() {return identifiers;}

        TValue& getX(){return x;}
        TValue& getY(){return y;}
        TValue& getWidth(){return width;}
        TValue& getHeight(){return height;}

        void setLayout(std::shared_ptr<UILayout> layout) {this->layout = layout;}
        std::shared_ptr<UILayout>& getLayout(){return layout;}

        void setHoverable(bool value) {hoverable = value;}
        void setFocusable(bool value) {focusable = value;}
        bool isFocusable(){return focusable;}
        bool isHoverable(){return hoverable;}
        bool isScrollable(){return scrollable;}
        bool isUnderHover(){return state == HOVER;}

        void update();
        void updateChildren();
        void stopDrawing();
        void stopDrawingChildren();

        virtual void appendChild(std::shared_ptr<UIFrame> child);
        virtual void clearChildren();

        template <typename T>
        T getAttribute(std::optional<T> Style::*attribute){
            auto& style = getStyleForState(state);
            if (style.*attribute) {
                return *(style.*attribute);
            }
            return *(baseStyle.*attribute);
        }

        template <typename T>
        void setAttribute(std::optional<T> Style::*attribute, T value, UIElementState state = BASE){
            auto& style = getStyleForState(state);
            style.*attribute = value;
        }

        const UITransform& getBoundingTransform() const {return boundingTransform;}
        const UITransform& getViewportTransform() const {return viewportTransform;}
        const UITransform& getContentTransform() const {return contentTransform;}

        virtual std::vector<std::shared_ptr<UIFrame>>& getChildren(){return children;}

        virtual void calculateElementsTransforms();
        virtual void calculateChildrenTransforms();
        virtual void calculateTransforms(); // calculates elements own transforms and then children transforms
};

class UILabel: public UIFrame{
    protected:
        std::string text;
        bool resizeToText = false;
        int textPadding = 5;

        UITransform getTextPosition();

    public:
        UILabel(){
            identifiers.tag = "label";
        }
        virtual void getRenderingInformation(UIRenderBatch& batch);

        void calculateElementsTransforms() override;

        void setText(std::string text) {this->text = text;}
        void setTextPadding(int padding) {this->textPadding = padding;}
        std::string& getText() {return text;}
};

class UIInput: public UILabel{
    private:

    public:
        UIInput();
 
        std::function<void(std::string)> onSubmit;

        virtual void getRenderingInformation(UIRenderBatch& batch);
};

class UISlider: public UIFrame{
    public:
        enum Orientation{
            VERTICAL,
            HORIZONTAL
        };

    private:
        int* value;
        uint min;
        uint max;

        bool displayValue = true;
        int valueDisplayOffset = 10;

        bool grabbed = false;
        Orientation orientation = HORIZONTAL;

        uint handleWidth = 15;
        UIColor handleColor = UIColor(0.361f, 0.443f, 0.741f,1.0f);

        UITransform getHandleTransform();
        void moveTo(glm::vec2 pos);

    public:
        UISlider();
        void setOrientation(Orientation value){orientation = value;}
        void setDisplayValue(bool value) {displayValue = value;}
        void setHandleWidth(uint width) {handleWidth = width;}

        void setMax(uint value) {max = value;}
        void setMin(uint value) {min = value;}
        void setValuePointer(int* value) {this->value = value;}

        std::function<void(void)> onMove;

        void getRenderingInformation(UIRenderBatch& batch) override;
};


class UIScrollableFrame: public UIFrame{
    private:
        std::shared_ptr<UISlider> slider;

        int sliderWidth = 15;

        int scroll = 0;
        int scrollMax = 1000;

    public:
        UIScrollableFrame();
        void calculateElementsTransforms() override;

};

class UIImage: public UIFrame{
    public:
        UIImage(std::string path);

        void getRenderingInformation(UIRenderBatch& batch) override;
};

#include <ui/core.hpp>