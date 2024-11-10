#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <memory>
#include <rendering/shaders.hpp>
#include <rendering/mesh.hpp>
#include <ui/font.hpp>
#include <queue>
#include <functional>
#include <optional>

/*
    Structure used for all colors in the ui, always RGBA
*/
struct UIColor{
    float r;
    float g;
    float b;
    float a;

    // Integers in ranges 0 - 255
    UIColor(int r, int g, int b, int a = 255){
        this->r = static_cast<float>(r) / 255.0f;
        this->g = static_cast<float>(g) / 255.0f;
        this->b = static_cast<float>(b) / 255.0f;
        this->a = static_cast<float>(a) / 255.0f;
    }

    // Floats in ranges 0.0f - 1.0f
    UIColor(float r, float g, float b, float a = 1.0f): r(r), g(g), b(b), a(a) {}

    // Adds the value to r,g and b components (clamps the color to be always valid after shifting)
    UIColor shifted(float value){
        return UIColor(
            glm::clamp(r + value, .0f, 1.0f),
            glm::clamp(g + value, .0f, 1.0f),
            glm::clamp(b + value, .0f, 1.0f),
            a
        );
    }
};

enum Units{
    NONE, // The default value always 0 pixels
    PIXELS, 

    WINDOW_WIDTH, // Percentage of the window width
    WINDOW_HEIGHT, // Percentage of the window height
    
    MY_PERCENT, // Percentage of the size of the widget
    /*
        The percentage of parent.
        Subtracts elements own margin and border sizes from the width, 100% is a perfect fit even with a margin and border
    */
    PERCENT,

    OPERATION_PLUS    , // TValue + TValue (resolved to pixels)
    OPERATION_MINUS   , // TValue - TValue (resolved to pixels)
    OPERATION_MULTIPLY, // TValue * TValue (resolved to pixels)
    OPERATION_DIVIDE  , // TValue / TValue (resolved to pixels)
};

struct TValue{
    Units unit = NONE;
    int value = 0; 

    std::vector<TValue> operands;

    TValue(Units unit, int value) : unit(unit), value(value){}
    TValue(int value) : unit(PIXELS), value(value){}

    // Gets automatically resolved if the operands have the same types
    TValue(Units operation, TValue op1, TValue op2): unit(operation){
        /*if(op1.unit == op2.unit){
            unit = op1.unit;
            switch(operation){
                case OPERATION_PLUS    : value = op1.value + op2.value; return;
                case OPERATION_MINUS   : value = op1.value - op2.value; return;
                case OPERATION_MULTIPLY: value = op1.value * op2.value; return;
                case OPERATION_DIVIDE  : value = op1.value / op2.value; return;
                default:
                    unit = operation;
                break;
            }
        }*/
        
        operands.push_back(op1);
        operands.push_back(op2);
    }

    bool hasParentReference(){
        if(unit == OPERATION_PLUS || unit == OPERATION_MINUS){
            return operands[0].hasParentReference() || operands[1].hasParentReference();
        }
        return unit == PERCENT;
    }
};

const static TValue TNONE = {NONE, 0};

struct UIRegion{
    glm::vec2 min;
    glm::vec2 max;
};

struct UITransform{
    int x;
    int y;
    int width;
    int height;

    UIRegion asRegion(){return {{x,y},{x + width, y + height}}; }
    std::string to_string(){return "Transform(" + std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(width) + "," + std::to_string(height) + ")";}
};

struct UIBorderSizes{
    int top;
    int right;
    int bottom;
    int left;
};

#define UI_VERTEX_SIZE (3 + 2 + 4 + 2 + 1 + 1 + 1 + 4 + 4 + 4 + 4 + 4 + 4)

using RawRenderInfo = std::array<float, 4 * UI_VERTEX_SIZE>;

struct UIRenderInfo{
    public:
        int x;
        int y;
        int width;
        int height;

        UIBorderSizes borderWidth; // clockwise from the top
        std::array<UIColor,4> borderColor;

        UIColor color;

        bool isText = false;
        bool isTexture = false;

        bool hasTexCoords = false;
        std::vector<glm::vec2> texCoords;
        int textureIndex;

        UIRegion clipRegion; // minX/Y, maxX/Y
        bool clip = false;

        int zIndex = 0;

        void process(Mesh* output);
        bool valid();

        static UIRenderInfo Rectangle(int x, int y, int width, int height, UIColor color, UIBorderSizes borderWidth = {0,0,0,0},std::array<UIColor,4> borderColor = {UIColor(0,0,0,0),{0,0,0,0},{0,0,0,0},{0,0,0,0}}){
            return {
                x,y,width,height,borderWidth,borderColor,color
            };
        }
        static UIRenderInfo Rectangle(UITransform t, UIColor color, UIBorderSizes borderWidth = {0,0,0,0},std::array<UIColor,4> borderColor = {UIColor(0,0,0,0),{0,0,0,0},{0,0,0,0},{0,0,0,0}}){
            return UIRenderInfo::Rectangle(t.x,t.y,t.width,t.height,color, borderWidth, borderColor);
        }
        static UIRenderInfo Text(int x, int y, int width, int height, UIColor color, std::vector<glm::vec2> texCoords){
            return {
                x,y,width,height,
                {0,0,0,0}, // Border thickness
                {UIColor(0,0,0,0),{0,0,0,0},{0,0,0,0},{0,0,0,0}}, // Border color
                color,
                true, // Is text
                false, // Isnt a texture
                true, // Has tex coords
                texCoords
            };
        }
        static UIRenderInfo Texture(int x, int y, int width, int height, std::vector<glm::vec2> texCoords, int textureIndex){
            return {
                x,y,width,height,
                {0,0,0,0},
                {UIColor(0,0,0,0),{0,0,0,0},{0,0,0,0},{0,0,0,0}},
                {0,0,0,1},
                false, // Isnt text
                true, // Is a texture
                true, // Has tex coords
                texCoords,
                textureIndex
            };
        }

        static std::array<UIColor, 4> generateBorderColors(UIColor base){
            return {
                base.shifted(0.1),
                base.shifted(0.1),
                base.shifted(-0.1),
                base.shifted(-0.1)
            };
        }
};

class UIManager;
class UIFrame;
class UILoader;
class UIStyle;

/*
    Functionaly very similar to yield in an iterator, but slightly different

    Named for the comedic value
*/
using RenderYeetFunction = std::function<void(UIRenderInfo info, UIRegion clipRegion)>;

/*
    Base class for all layouts
*/
class UILayout{
    public:
        /*
            Resizes or changes the element itself
        */
        virtual void arrangeSelf(UIFrame* frame) {};
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
        FlexDirection direction;
        bool expandToChildren = false;

    public:
        void setExpand(bool value) {expandToChildren = value;}
        void setDirection(FlexDirection direction) {this->direction = direction;}
        
        void arrangeSelf(UIFrame* frame) override;
        void arrangeChildren(UIFrame* frame) override;
};
/*
    Core element that every other element inherits from
*/
class UIFrame{
    public:
        enum State{
            BASE,
            HOVER,
            FOCUS
        };
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
            std::optional<std::array<UIColor,4>> borderColor;
            std::optional<TValue>                margin;
        };

    protected:
        UIManager& manager;

        Style baseStyle = {
            Style::TextPosition::LEFT,
            UIColor{255,255,255,255},
            UIColor{0,0,0,0},
            std::array<TValue,4>{0,0,0,0},
            std::array<UIColor,4>{UIColor{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
            TValue(0)
        };
        Style hoverStyle;
        Style focusStyle;

        struct Identifiers{
            std::string tag = "";
            std::vector<std::string> classes = {};
            std::string id = "";
        } identifiers;

        std::shared_ptr<UILayout> layout;

        Style& getStyleForState(State state){
            switch(state){
                case BASE : return baseStyle;
                case HOVER: return hoverStyle;
                case FOCUS: return focusStyle;
            }
            return baseStyle;
        }

        State state;

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
        UITransform contentTransform  = {0,0,0,0}; // Transform for only content
        UITransform boundingTransform = {0,0,0,0}; // Transform that includes margin
        UIBorderSizes borderSizes     = {0,0,0,0};
        UIRegion clipRegion           = {{0,0},{0,0}};
        UIRegion contentClipRegion    = {{0,0},{0,0}};
        int margin_x = 0;
        int margin_y = 0;

        virtual void getRenderingInformation(RenderYeetFunction& yeet);

        friend class UIManager;
        friend class UILoader;
        friend class UIStyle;

    public:
        UIFrame(UIManager& manager): manager(manager) {}
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

        void setIdentifiers(std::string tag, std::vector<std::string> classes = {}, std::string id = "") {identifiers = {tag,classes,id};}
        
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

        virtual void appendChild(std::shared_ptr<UIFrame> child){
            child->parent = this;
            child->zIndex = this->zIndex + 1;
            children.push_back(child);

            child->calculateTransforms();
        }
        virtual void clearChildren(){
            children.clear();
        }

        template <typename T>
        T getAttribute(std::optional<T> Style::*attribute){
            auto& style = getStyleForState(state);
            if (style.*attribute) {
                return *(style.*attribute);
            }
            return *(baseStyle.*attribute);
        }

        template <typename T>
        void setAttribute(std::optional<T> Style::*attribute, T value, State state = BASE){
            auto& style = getStyleForState(state);
            style.*attribute = value;
        }

        const UITransform& getBoundingTransform() const {return boundingTransform;}
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
        UILabel(UIManager& manager): UIFrame(manager) {}
        virtual void getRenderingInformation(RenderYeetFunction& yeet);

        void setText(std::string text) {this->text = text;}
        void setTextPadding(int padding) {this->textPadding = padding;}
        std::string& getText() {return text;}
};

class UIInput: public UILabel{
    private:

    public:
        UIInput(UIManager& manager);
 
        std::function<void(std::string)> onSubmit;

        virtual void getRenderingInformation(RenderYeetFunction& yeet);
};

class UIImage: public UIFrame{
    private:
        std::string path;
        bool loaded = false;

    public:
        UIImage(UIManager& manager): UIFrame(manager) {}
        void loadFromFile(std::string path);
        virtual void getRenderingInformation(RenderYeetFunction& yeet);
};

class UISlider: public UIFrame{
    public:
        enum Orientation{
            VERTICAL,
            HORIZONTAL
        };

    private:
        int* value;
        uint32_t min;
        uint32_t max;

        bool displayValue = true;
        int valueDisplayOffset = 10;

        bool grabbed = false;
        Orientation orientation = HORIZONTAL;

        uint32_t handleWidth = 15;
        UIColor handleColor = UIColor(0.361f, 0.443f, 0.741f,1.0f);

        UITransform getHandleTransform(UIManager& manager);
        void moveTo(UIManager& manager, glm::vec2 pos);

    public:
        UISlider(UIManager& manager);
        void setOrientation(Orientation value){orientation = value;}
        void setDisplayValue(bool value) {displayValue = value;}
        void setHandleWidth(uint32_t width) {handleWidth = width;}

        void setMax(uint32_t value) {max = value;}
        void setMin(uint32_t value) {min = value;}
        void setValuePointer(int* value) {this->value = value;}

        std::function<void(void)> onMove;

        virtual void getRenderingInformation(RenderYeetFunction& yeet);
};


class UIScrollableFrame: public UIFrame{
    private:
        std::shared_ptr<UIFrame> body;
        std::shared_ptr<UISlider> slider;

        int sliderWidth = 15;

        int scroll = 0;
        int scrollMax = 1000;
    public:
        UIScrollableFrame(UIManager& manager);
        void appendChild(std::shared_ptr<UIFrame> child) override {body->appendChild(child);};
        void clearChildren() override {body->clearChildren();}

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
        uint32_t cursorMode =  GLFW_CURSOR_NORMAL;
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

using UIWindowIdentifier = uint32_t;

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

class UIManager{
    private:
        FontManager fontManager;
        std::unique_ptr<Font> mainFont;

        ShaderProgram uiProgram;
        VertexFormat vertexFormat;

        std::unique_ptr<GLBuffer> drawBuffer;

        Uniform<glm::mat4> projectionMatrix = Uniform<glm::mat4>("projectionMatrix");
        
        int screenWidth = 1920;
        int screenHeight = 1080;

        glm::ivec2 mousePosition = {0,0};

        std::shared_ptr<UIFrame> underHover;
        std::shared_ptr<UIFrame> inFocus;
        std::shared_ptr<UIFrame> underScrollHover;

        UIWindowIdentifier currentWindow = -1;
        UIWindowIdentifier lastWindowIndentifier = 0;
        std::unordered_map<UIWindowIdentifier, UIWindow> windows;

        std::unique_ptr<DynamicTextureArray> textures;

    public:
        void initialize();
        void resize(int width, int height);
        void update();
        void setFocus(std::shared_ptr<UIFrame> ptr){inFocus = ptr;}

        void mouseMove(int x, int y);
        void mouseEvent(GLFWwindow* window, int button, int action, int mods);
        void keyTypedEvent(GLFWwindow* window, unsigned int codepoint);
        void keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods);
        void scrollEvent(GLFWwindow* window, double xoffset, double yoffset);

        void resetStates(); // Resets current elements in focus and hover to be none

        void render();
        void setCurrentWindow(UIWindowIdentifier id);
        UIWindow& getCurrentWindow();
        UIWindow& getWindow(UIWindowIdentifier id);
        UIWindowIdentifier createWindow();

        std::shared_ptr<UIFrame> getElementUnder(int x, int y, bool onlyScrollable = false);   

        void buildTextRenderingInformation(RenderYeetFunction& yeet, UIRegion& clipRegion, std::string text, float x, float y, float scale, UIColor color);

        Uniform<glm::mat4>& getProjectionMatrix(){return projectionMatrix;}
        FontManager& getFontManager() {return fontManager;};
        Font& getMainFont(){return *mainFont;}

        glm::ivec2 getMousePosition(){return mousePosition;}

        int getScreenWidth() {return screenWidth;}
        int getScreenHeight() {return screenHeight;}

        std::unique_ptr<DynamicTextureArray>& getTextures(){return textures;}

        // Creates an element that belongs to the UIManager
        template <typename T>
        std::shared_ptr<T> createElement(){
            return std::make_shared<T>(*this);
        }
};

#endif