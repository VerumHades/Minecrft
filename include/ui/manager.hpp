#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <memory>
#include <rendering/opengl/shaders.hpp>
#include <rendering/mesh.hpp>
#include <ui/font.hpp>
#include <queue>
#include <functional>
#include <optional>
#include <unordered_set>

#include <ui/color.hpp>
#include <ui/tvalue.hpp>
#include <ui/loader.hpp>
#include <ui/layouts.hpp>
#include <ui/backend.hpp>

class UIManager;
class UIFrame;
class UILoader;
class UIStyle;

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

            std::optional<TextPosition>          textPosition;
            std::optional<UIColor>               textColor;
            std::optional<UIColor>               backgroundColor;
            std::optional<std::array<TValue,4>>  borderWidth;
            std::optional<UIBorderColors>        borderColor;
            std::optional<TValue>                margin;
            std::optional<TValue>                fontSize;
        };

    protected:
        UIManager& manager;

        Style baseStyle = {
            Style::TextPosition::LEFT,
            UIColor{255,255,255,255},
            UIColor{0,0,0,0},
            std::array<TValue,4>{0,0,0,0},
            UIBorderColors{UIColor{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
            TValue(0),
            24
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
        TValue width = TNONE;
        TValue height = TNONE;

        int zIndex = 0;
        std::vector<size_t> renderDataLocations;

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
        UIBorderSizes borderSizes     = {0,0,0,0};
        UIRegion clipRegion           = {{0,0},{0,0}};
        UIRegion contentClipRegion    = {{0,0},{0,0}};
        int margin_x = 0;
        int margin_y = 0;
        int font_size = 0;

        virtual void getRenderingInformation(UIRenderBatch& batch);

        std::shared_ptr<GLTextureArray> dedicated_texture_array;

        friend class UIManager;
        friend class UILoader;
        friend class UIStyle;

        bool has_draw_batch = false;
        std::list<UIBackend::Batch>::iterator draw_batch_iterator;

    public:
        UIFrame(UIManager& manager): manager(manager) {
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
        std::function<void(UIManager& manager, int, int, int)> onMouseEvent;
        std::function<void(UIManager& manager, int, int)> onMouseMove;
        std::function<void(GLFWwindow*, unsigned int)> onKeyTyped;
        std::function<void(GLFWwindow*, int key, int scancode, int action, int mods)> onKeyEvent;
        std::function<void(void)> onClicked;

        std::function<void(UIManager& manager)> onMouseLeave;
        std::function<void(UIManager& manager)> onMouseEnter;

        std::function<void(UIManager& manager, int offsetX, int offsetY)> onScroll;

        void setPosition(TValue x, TValue y){this->x = x; this->y = y;}
        void setX(TValue x) {this->x = x;}
        void setY(TValue y) {this->y = y;}

        void setSize(TValue width, TValue height) {this->width = width; this->height = height;}
        void setWidth(TValue width) {this->width = width;}
        void setHeight(TValue height) {this->height = height;}

        void setIdentifiers(std::unordered_set<std::string> classes = {}, std::string id = "") {identifiers = {identifiers.tag,classes,id};}
        Identifiers& getIdentifiers() {return identifiers;}

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

        UITransform getTextPosition(UIManager& manager);

    public:
        UILabel(UIManager& manager): UIFrame(manager) {identifiers.tag = "label";}
        virtual void getRenderingInformation(UIRenderBatch& batch);

        void setText(std::string text) {this->text = text;}
        void setTextPadding(int padding) {this->textPadding = padding;}
        std::string& getText() {return text;}
};

class UIInput: public UILabel{
    private:

    public:
        UIInput(UIManager& manager);
 
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

        UITransform getHandleTransform(UIManager& manager);
        void moveTo(UIManager& manager, glm::vec2 pos);

    public:
        UISlider(UIManager& manager);
        void setOrientation(Orientation value){orientation = value;}
        void setDisplayValue(bool value) {displayValue = value;}
        void setHandleWidth(uint width) {handleWidth = width;}

        void setMax(uint value) {max = value;}
        void setMin(uint value) {min = value;}
        void setValuePointer(int* value) {this->value = value;}

        std::function<void(void)> onMove;

        virtual void getRenderingInformation(UIRenderBatch& batch);
};


class UIScrollableFrame: public UIFrame{
    private:
        std::shared_ptr<UISlider> slider;

        int sliderWidth = 15;

        int scroll = 0;
        int scrollMax = 1000;
    public:
        UIScrollableFrame(UIManager& manager);
        void calculateElementsTransforms() override;

};

struct UIEventLock{
    bool keyEvent = false;
    bool mouseEvent = false;
    bool mouseMove = false;
    bool scrollEvent = false;
};

class UILayer{
    private:
        std::vector<std::shared_ptr<UIFrame>> elements;
        std::unordered_map<std::string, std::shared_ptr<UIFrame>> idRegistry;
        
    public:
        uint cursorMode =  GLFW_CURSOR_NORMAL;
        UIEventLock eventLocks = {};

        void clear(){elements.clear();}
        void addElement(std::shared_ptr<UIFrame> element){
            elements.push_back(element);
            element->calculateTransforms();
        }
        void addElementWithID(std::string id, std::shared_ptr<UIFrame> element){
            idRegistry[id] = element;
        }
        std::shared_ptr<UIFrame> getElementById(std::string id){
            if(idRegistry.count(id) == 0) {
                std::cerr << "No element under id: " << id << std::endl;
            }
            return idRegistry[id];
        }
        std::vector<std::shared_ptr<UIFrame>>& getElements() {return elements;}
};

using UIWindowIdentifier = int;

class UIWindow{
    private: 
        std::unordered_map<std::string, UILayer> layers;
        std::string currentLayer = "default";

    public:
        void setCurrentLayer(std::string name) {currentLayer = name;};
        std::string getCurrentLayerName(){return currentLayer;}
        UILayer& getCurrentLayer() {return layers[currentLayer];}
        UILayer& getLayer(std::string name) {return layers[name];}
};

#include <ui/loader.hpp>

class UIManager{
    private:
        int screenWidth = 1920;
        int screenHeight = 1080;

        glm::ivec2 mousePosition = {0,0};

        std::shared_ptr<UIFrame> underHover;
        std::shared_ptr<UIFrame> inFocus;
        std::shared_ptr<UIFrame> underScrollHover;

        UIWindowIdentifier currentWindow = -1;
        std::vector<UIWindow> windows;

        void renderElementAndChildren(std::shared_ptr<UIFrame>& element);

        UILoader loader;
        UIBackend* backend;

    public:
        UIManager(UIBackend* backend);
        void resize(int width, int height);
        void setFocus(std::shared_ptr<UIFrame> ptr){inFocus = ptr;}

        void mouseMove(int x, int y);
        void mouseEvent(GLFWwindow* window, int button, int action, int mods);
        void keyTypedEvent(GLFWwindow* window, unsigned int codepoint);
        void keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods);
        void scrollEvent(GLFWwindow* window, double xoffset, double yoffset);

        void resetStates(); // Resets current elements in focus and hover to be none
        void updateAll(); // Updates all elements (might be slow)
        void stopDrawingAll();

        void render();
        void setCurrentWindow(UIWindowIdentifier id);
        UIWindow* getCurrentWindow();
        UIWindow* getWindow(UIWindowIdentifier id);
        UIWindowIdentifier createWindow();
        void loadWindowFromXML(UIWindow& window, std::string load_path);

        UILoader& getLoader() {return loader;}
        UIBackend* getBackend() {return backend;}

        std::shared_ptr<UIFrame> getElementUnder(int x, int y, bool onlyScrollable = false);   

        glm::ivec2 getMousePosition(){return mousePosition;}

        int getScreenWidth() {return screenWidth;}
        int getScreenHeight() {return screenHeight;}

        // Creates an element that belongs to the UIManager
        template <typename T>
        std::shared_ptr<T> createElement(){
            return std::make_shared<T>(*this);
        }
};

#endif