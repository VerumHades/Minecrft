#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <memory>
#include <rendering/shaders.hpp>
#include <rendering/mesh.hpp>
#include <ui/font.hpp>
#include <queue>
#include <functional>
#include <optional>
#include <unordered_set>

#include <ui/color.hpp>
#include <ui/tvalue.hpp>
#include <ui/loader.hpp>


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

        void process(Mesh* output, size_t offset_index = 0);
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
        FlexDirection direction;
        bool expandToChildren = false;

    public:
        void setExpand(bool value) {expandToChildren = value;}
        void setDirection(FlexDirection direction) {this->direction = direction;}
        
        UITransform calculateContentTransform(UIFrame* frame) override;
        void arrangeChildren(UIFrame* frame) override;
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

            std::optional<TextPosition>          textPosition;
            std::optional<UIColor>               textColor;
            std::optional<UIColor>               backgroundColor;
            std::optional<std::array<TValue,4>>  borderWidth;
            std::optional<std::array<UIColor,4>> borderColor;
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
            std::array<UIColor,4>{UIColor{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
            TValue(0),
            16
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

        size_t vertex_data_start = -1ULL;
        size_t vertex_data_size = 0;
        size_t index_data_start  = -1ULL;
        size_t index_data_size = 0;

        virtual void getRenderingInformation(RenderYeetFunction& yeet);

        std::shared_ptr<GLTextureArray> dedicated_texture_array;

        friend class UIManager;
        friend class UILoader;
        friend class UIStyle;

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
        UIImage(UIManager& manager): UIFrame(manager) {identifiers.tag = "image";}
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

        virtual void getRenderingInformation(RenderYeetFunction& yeet);
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
        FontManager fontManager;
        std::unique_ptr<Font> mainFont;

        ShaderProgram uiProgram = ShaderProgram("shaders/graphical/ui/ui.vs","shaders/graphical/ui/ui.fs");
        VertexFormat vertexFormat;

        GLVertexArray vao;
        GLAllocatedBuffer<uint,  GL_ELEMENT_ARRAY_BUFFER> indexBuffer;
        GLAllocatedBuffer<float, GL_ARRAY_BUFFER> vertexBuffer;
        size_t vertexSize;

        Uniform<glm::mat4> projectionMatrix = Uniform<glm::mat4>("ui_projection_matrix");
        
        int screenWidth = 1920;
        int screenHeight = 1080;

        glm::ivec2 mousePosition = {0,0};

        std::shared_ptr<UIFrame> underHover;
        std::shared_ptr<UIFrame> inFocus;
        std::shared_ptr<UIFrame> underScrollHover;

        UIWindowIdentifier currentWindow = -1;
        std::vector<UIWindow> windows;

        std::unique_ptr<DynamicTextureArray> textures;

        void renderElementAndChildren(std::shared_ptr<UIFrame>& element, uint& boundTexture);

        UILoader loader;

    public:
        UIManager();
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

        std::shared_ptr<UIFrame> getElementUnder(int x, int y, bool onlyScrollable = false);   

        void buildTextRenderingInformation(RenderYeetFunction& yeet, UIRegion& clipRegion, std::string text, float x, float y, int font_size, UIColor color);

        FontManager& getFontManager() {return fontManager;};
        Font& getMainFont(){return *mainFont;}

        glm::ivec2 getMousePosition(){return mousePosition;}

        int getScreenWidth() {return screenWidth;}
        int getScreenHeight() {return screenHeight;}

        std::unique_ptr<DynamicTextureArray>& getTextures(){return textures;}

        auto& getIndexBuffer()  { return indexBuffer;  }
        auto& getVertexBuffer() { return vertexBuffer; }

        // Creates an element that belongs to the UIManager
        template <typename T>
        std::shared_ptr<T> createElement(){
            return std::make_shared<T>(*this);
        }
};

#endif