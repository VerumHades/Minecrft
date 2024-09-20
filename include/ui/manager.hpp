#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <memory>
#include <rendering/shaders.hpp>
#include <rendering/mesh.hpp>
#include <ui/font.hpp>
#include <queue>

struct UIRenderInfo{
    int x;
    int y;
    int width;
    int height;

    bool isText = false;
    glm::vec3 color;

    bool hasTexCoords = false;
    std::vector<glm::vec2> texCoords;
};

class UIFrame{
    private:
        int x;
        int y;
        int width;
        int height;

        glm::vec3 color;

        std::vector<UIFrame> children;

    public:
        UIFrame(int x, int y,int width,int height,glm::vec3 color): x(x), y(y), width(width), height(height), color(color) {}
        virtual std::vector<UIRenderInfo> getRenderingInformation();
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

        std::vector<UIFrame> windows;

        void processRenderingInformation(UIRenderInfo& info, Mesh& output);
        std::vector<UIRenderInfo> buildTextRenderingInformation(std::string text, float x, float y, float scale, glm::vec3 color);

    public:
        void initialize();
        void resize(int width, int height);
        void update();
        void mouseEvent(int x, int y, int state);
        void mouseMove(int x, int y);
        void draw();  

        Uniform<glm::mat4>& getProjectionMatrix(){return projectionMatrix;}
        FontManager& getFontManager() {return fontManager;};
};

#endif