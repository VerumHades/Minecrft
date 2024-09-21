#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <memory>
#include <rendering/shaders.hpp>
#include <rendering/mesh.hpp>
#include <ui/font.hpp>
#include <queue>

enum Units{
    PIXELS,
    FRACTIONS, // Percentage of the window
    OPERATION_PLUS, // TValue + TValue (resolved to pixels)
    OPERATION_MINUS, // TValue - TValue
    MFRACTION // Percentage of the size of the widget
};

struct TValue{
    Units unit;
    int value;

    std::vector<TValue> operands;

    TValue(Units unit, int value) : unit(unit), value(value){}
    TValue(Units operation, TValue op1, TValue op2): unit(operation){
        operands.push_back(op1);
        operands.push_back(op2);
    }
};

struct UIRenderInfo{
    TValue x;
    TValue y;
    TValue width;
    TValue height;

    bool isText = false;
    glm::vec3 color;

    bool hasTexCoords = false;
    std::vector<glm::vec2> texCoords;
};

class UIManager;

class UIFrame{
    protected:
        TValue x;
        TValue y;
        TValue width;
        TValue height;

        glm::vec3 color;

        std::vector<std::unique_ptr<UIFrame>> children;

    public:
        UIFrame(TValue x, TValue y, TValue width, TValue height,glm::vec3 color): x(x), y(y), width(width), height(height), color(color) {}
        virtual std::vector<UIRenderInfo> getRenderingInformation(UIManager& manager);

        int getValueInPixels(TValue& value, bool horizontal, int container_size);
};

class UILabel: public UIFrame{
    private:
        std::string text;
        TValue padding = {PIXELS, 4};

    public:
        UILabel(std::string text, TValue x, TValue y, glm::vec3 color): UIFrame(x,y,{PIXELS, 0},{PIXELS, 0},color), text(text) {}
        std::vector<UIRenderInfo> getRenderingInformation(UIManager& manager) override;
};

class UIManager{
    private:
        ShaderProgram uiProgram;
        FontManager fontManager;
        std::unique_ptr<Font> mainFont;
        std::unique_ptr<GLBuffer> drawBuffer;
        Uniform<glm::mat4> projectionMatrix = Uniform<glm::mat4>("projectionMatrix");
        
        int screenWidth = 1920;
        int screenHeight = 1080;

        std::vector<std::unique_ptr<UIFrame>> windows;

        void processRenderingInformation(UIRenderInfo& info, UIFrame& frame, Mesh& output);

    public:
        void initialize();
        void resize(int width, int height);
        void update();
        void mouseEvent(int x, int y, int state);
        void mouseMove(int x, int y);
        void draw();  

        std::vector<UIRenderInfo> buildTextRenderingInformation(std::string text, float x, float y, float scale, glm::vec3 color);

        Uniform<glm::mat4>& getProjectionMatrix(){return projectionMatrix;}
        FontManager& getFontManager() {return fontManager;};
        Font& getMainFont(){return *mainFont;}

        int getScreenWidth() {return screenWidth;}
        int getScreenHeight() {return screenHeight;}
};

#endif