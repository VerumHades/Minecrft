#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <memory>
#include <rendering/shaders.hpp>
#include <rendering/mesh.hpp>
#include <queue>

struct UIRenderInfo{
    int x;
    int y;
    int width;
    int height;
};

class UIFrame{
    private:
        int x;
        int y;
        int width;
        int height;

    public:
        UIFrame(int x, int y,int width,int height): x(x), y(y), width(width), height(height) {}
        UIRenderInfo getRenderingInformation(){
            return {
                x,y,width,height
            };
        };
};

class UIManager{
    private:
        std::unique_ptr<ShaderProgram> uiProgram;
        std::unique_ptr<GLBuffer> drawBuffer;
        Uniform<glm::mat4> projectionMatrix = Uniform<glm::mat4>("projectionMatrix");
        
        int screenWidth = 1920;
        int screenHeight = 1080;

        std::vector<UIFrame> windows;

        void processRenderingInformation(UIRenderInfo& info, Mesh& output);

    public:
        void initialize();
        void resize(int width, int height);
        void update();
        void draw();  
};

#endif