#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <memory>
#include <rendering/shaders.hpp>
#include <rendering/mesh.hpp>
#include <ui/font.hpp>
#include <queue>
#include <functional>

using UIColor = glm::vec4;

enum Units{
    PIXELS,
    FRACTIONS, // Percentage of the window
    OPERATION_PLUS, // TValue + TValue (resolved to pixels)
    OPERATION_MINUS, // TValue - TValue (resolved to pixels)
    MFRACTION, // Percentage of the size of the widget
    PFRACTION // Percentage of parrent
};

struct TValue{
    Units unit;
    int value;

    std::vector<TValue> operands;

    TValue(Units unit, int value) : unit(unit), value(value){}
    TValue(int value) : unit(PIXELS), value(value){}
    TValue(Units operation, TValue op1, TValue op2): unit(operation){
        operands.push_back(op1);
        operands.push_back(op2);
    }

    TValue operator-(const TValue& other) const {
        return TValue(OPERATION_MINUS, *this, other);
    }

    TValue operator-() const {
        return TValue(unit, -value);
    }
};

// Pixels
TValue operator"" px(unsigned long long value);
// Percent of parent
TValue operator"" ps(unsigned long long value);
// Percent of widget
TValue operator"" ws(unsigned long long value);

struct UITransform{
    int x;
    int y;
    int width;
    int height;
};

struct UIBorderSizes{
    int top;
    int right;
    int bottom;
    int left;
};

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

        glm::vec4 clipRegion; // minX/Y, maxX/Y
        bool clip = false;

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
                glm::clamp(UIColor(glm::vec3(base) + 0.1f, base.a),0.0f,1.0f),
                glm::clamp(UIColor(glm::vec3(base) + 0.1f, base.a),0.0f,1.0f),
                glm::clamp(UIColor(glm::vec3(base) - 0.1f, base.a),0.0f,1.0f),
                glm::clamp(UIColor(glm::vec3(base) - 0.1f, base.a),0.0f,1.0f)
            };
        }
};

class UIManager;

class UIFrame{
    protected:
        TValue x;
        TValue y;
        TValue width;
        TValue height;

        std::vector<TValue> borderWidth = {{PIXELS,3},{PIXELS,3},{PIXELS,3},{PIXELS,3}};

        bool hover = false;
        bool focusable = false;
        bool hoverable = true;

        UIColor color;
        UIColor hoverColor = glm::vec4(0.0,0.1,0.5,0.0);
        std::array<UIColor,4> borderColor = {
            glm::vec4(0.5,0.1,0.5,0.4),
            glm::vec4(0.5,0.1,0.5,0.4),
            glm::vec4(0.5,0.1,0.5,0.4),
            glm::vec4(0.5,0.1,0.5,0.4)
        };

        std::vector<std::shared_ptr<UIFrame>> children;
        UIFrame* parent = nullptr;

    public:
        UIFrame(TValue x, TValue y, TValue width, TValue height, UIColor color): x(x), y(y), width(width), height(height), color(color), hoverColor(color){
            borderColor = UIRenderInfo::generateBorderColors(color);
        }

        UIFrame(TValue width, TValue height, UIColor color): x(0), y(0), width(width), height(height), color(color), hoverColor(color){
            borderColor = UIRenderInfo::generateBorderColors(color);
        }
        virtual std::vector<UIRenderInfo> getRenderingInformation(UIManager& manager);

        std::function<void(UIManager& manager, int, int, int)> onMouseEvent;
        std::function<void(UIManager& manager, int, int)> onMouseMove;
        std::function<void(GLFWwindow*, unsigned int)> onKeyTyped;
        std::function<void(GLFWwindow*, int key, int scancode, int action, int mods)> onKeyEvent;
        std::function<void(void)> onClicked;

        std::function<void(UIManager& manager)> onMouseLeave;
        std::function<void(UIManager& manager)> onMouseEnter;

        int getValueInPixels(TValue& value, bool horizontal, int container_size);
        UITransform getTransform(UIManager& manager);
        UIBorderSizes getBorderSizes(UIManager& manager);
        glm::vec4 getClipRegion(UIManager& manager);

        void setHoverColor(UIColor color) {hoverColor = color;}
        void setBorderColor(std::array<UIColor,4> borderColor) {this->borderColor = borderColor;}
        void setColor(UIColor color) {this->color = color;}

        bool pointWithin(glm::vec2 position, UIManager& manager, int padding = 0);
        bool pointWithinBounds(glm::vec2 position, UITransform transform, int padding = 0);
        void setBorderWidth(std::vector<TValue> borderWidth) {this->borderWidth = borderWidth;}
        void setBorderWidth(TValue borderWidth) {this->borderWidth = {borderWidth,borderWidth,borderWidth,borderWidth};}

        void setPosition(TValue x, TValue y){this->x = x; this->y = y;}
        void setSize(TValue width, TValue height) {this->width = width; this->height = height;}

        void setHover(bool value) {hover = value;}
        void setHoverable(bool value) {hoverable = value;}
        void setFocusable(bool value) {focusable = value;}
        bool isFocusable(){return focusable;}
        bool isHoverable(){return hoverable;}

        virtual void appendChild(std::shared_ptr<UIFrame> child){
            child->parent = this;
            children.push_back(child);
        }
        void clearChildren(){
            children.clear();
        }
        std::vector<std::shared_ptr<UIFrame>>& getChildren() {return children;}
};

enum UITextPosition{
    LEFT,
    RIGHT,
    CENTER
};

class UILabel: public UIFrame{
    protected:
        std::string text;
        int textPadding = 5;
        UITextPosition textPosition = CENTER;

        UITransform getTextPosition(UIManager& manager);

    public:
        UILabel(std::string text, TValue x, TValue y, TValue width, TValue height, UIColor color): UIFrame(x,y,width,height,color), text(text) {}
        UILabel(std::string text, TValue width, TValue height, UIColor color): UIFrame(width,height,color), text(text) {}
        std::vector<UIRenderInfo> getRenderingInformation(UIManager& manager) override;

        void setText(std::string text) {this->text = text;}
        void setTextPosition(UITextPosition position) {this->textPosition = position;}
        void setTextPadding(int padding) {this->textPadding = padding;}
        std::string& getText() {return text;}
};

class UIInput: public UILabel{
    private:

    public:
        UIInput(TValue x, TValue y, TValue width, TValue height, UIColor color);

        std::function<void(std::string)> onSubmit;

        std::vector<UIRenderInfo> getRenderingInformation(UIManager& manager) override;
};

class UIImage: public UIFrame{
    private:
        std::string path;

    public:
        static std::unique_ptr<DynamicTextureArray> textures;

        UIImage(std::string path, TValue x, TValue y, TValue width, TValue height);
        std::vector<UIRenderInfo> getRenderingInformation(UIManager& manager) override;
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
        UIColor handleColor = glm::vec4(0.361, 0.443, 0.741,1.0);

        UITransform getHandleTransform(UIManager& manager);
        void moveTo(UIManager& manager, glm::vec2 pos);

    public:
        static std::unique_ptr<DynamicTextureArray> textures;

        UISlider(TValue x, TValue y, TValue width, TValue height, int* value, uint32_t min, uint32_t max, UIColor color);
        void setOrientation(Orientation value){orientation = value;}
        std::vector<UIRenderInfo> getRenderingInformation(UIManager& manager) override;
};

class UIFlexFrame: public UIFrame{
    public:
        enum FlexDirection{
            COLUMN,
            ROWS,
        };

    private:
        FlexDirection direction;
        TValue elementMargin = {0};

    public:

        UIFlexFrame(TValue x, TValue y, TValue width, TValue height, UIColor color): UIFrame(x,y,width,height,color) {};
        void setElementDirection(FlexDirection direction) {this->direction = direction;}
        void setElementMargin(TValue margin) {elementMargin = margin;}
        std::vector<UIRenderInfo> getRenderingInformation(UIManager& manager) override;
};

class UIScrollableFrame: public UIFrame{
    private:
        std::shared_ptr<UIFrame> internal;
    public:
        void appendChild(std::shared_ptr<UIFrame> child) override {internal->appendChild(child);};

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
        
    public:
        uint32_t cursorMode =  GLFW_CURSOR_NORMAL;
        UIEventLock eventLocks = {};

        void clear(){elements.clear();}
        void addElement(std::shared_ptr<UIFrame> element){
            elements.push_back(element);
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
        std::unique_ptr<GLBuffer> drawBuffer;
        Uniform<glm::mat4> projectionMatrix = Uniform<glm::mat4>("projectionMatrix");
        
        int screenWidth = 1920;
        int screenHeight = 1080;

        glm::ivec2 mousePosition = {0,0};

        UIFrame* underHover;
        UIFrame* inFocus;

        UIWindowIdentifier currentWindow = -1;
        UIWindowIdentifier lastWindowIndentifier = 0;
        std::unordered_map<UIWindowIdentifier, UIWindow> windows;

        void processRenderingInformation(UIRenderInfo& info, UIFrame& frame, Mesh& output);

    public:
        void initialize();
        void resize(int width, int height);
        void update();
        void setFocus(UIFrame* ptr){inFocus = ptr;}

        void mouseMove(int x, int y);
        void mouseEvent(GLFWwindow* window, int button, int action, int mods);
        void keyTypedEvent(GLFWwindow* window, unsigned int codepoint);
        void keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods);
        //void scrollEvent(int yoffset);

        void render();
        void setCurrentWindow(UIWindowIdentifier id);
        UIWindow& getCurrentWindow();
        UIWindow& getWindow(UIWindowIdentifier id);
        UIWindowIdentifier createWindow();

        UIFrame* getElementUnder(int x, int y);  

        std::vector<UIRenderInfo> buildTextRenderingInformation(std::string text, float x, float y, float scale, UIColor color);

        Uniform<glm::mat4>& getProjectionMatrix(){return projectionMatrix;}
        FontManager& getFontManager() {return fontManager;};
        Font& getMainFont(){return *mainFont;}

        glm::ivec2 getMousePosition(){return mousePosition;}

        int getScreenWidth() {return screenWidth;}
        int getScreenHeight() {return screenHeight;}
};

#endif