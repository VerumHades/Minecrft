#ifndef SCENE_H
#define SCENE_H

#include <memory>
#include <rendering/shaders.hpp>
#include <rendering/mesh.hpp>
#include <ui/font.hpp>
#include <ui/manager.hpp>
#include <queue>

class SceneManager;

class Scene{
    public:
        UIWindowIdentifier windowID;
        UIManager* uiManager;
        SceneManager* sceneManager;

        bool fpsLock = true;
        int targetFPS = 60;

        void setUILayer(std::string name);
        UILayer& getCurrentUILayer();
        UILayer& getUILayer(std::string name);
        void addElement(std::shared_ptr<UIFrame> element);
        UIWindow& getWindow();
        
        virtual void render() {};
        virtual void open(GLFWwindow* window)  {};
        virtual void close(GLFWwindow* window)  {};
        virtual void resize(GLFWwindow* /*window*/, int /*width*/, int /*height*/)  {};

        virtual void mouseMove(GLFWwindow* /*window*/, int /*x*/, int /*y*/)  {};
        virtual void mouseEvent(GLFWwindow* /*window*/, int /*button*/, int /*action*/, int /*mods*/)  {};
        virtual void scrollEvent(GLFWwindow* /*window*/, double /*xoffset*/, double /*yoffset*/) {};

        virtual void keyEvent(GLFWwindow* /*window*/, int /*key*/, int /*scancode*/, int /*action*/, int /*mods*/) {};
        /*
            This event cannot be locked by the ui manager and will always capture
        */
        virtual void unlockedKeyEvent(GLFWwindow* /*window*/, int /*key*/, int /*scancode*/, int /*action*/, int /*mods*/) {};
};

class SceneManager{ 
    private:
        std::string currentScene = "internal_default";
        std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
        Scene* getCurrentScene();

        UIManager manager;
        GLFWwindow* window;

        UIEventLock eventLocks;

    public:
        SceneManager();
        void initialize();
        void addScene(std::string name, std::unique_ptr<Scene> scene);
        void setScene(std::string name);
        Scene* getScene(std::string name);
        void setWindow(GLFWwindow* window) {this->window = window;}

        void resize(GLFWwindow* window, int width, int height);
        void render();

        void mouseMove(GLFWwindow* window, int x, int y);
        void mouseEvent(GLFWwindow* window, int button, int action, int mods);
        void scrollEvent(GLFWwindow* window, double xoffset, double yoffset);

        void keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods);
        void keyTypedEvent(GLFWwindow* window, unsigned int codepoint);

        GLFWwindow* getGLFWWindow(){return window;}
        void setEventLocks(const UIEventLock& locks) {eventLocks = locks;};
        float getTickTime() {return 1.0f / getCurrentScene()->targetFPS;};
        bool isFPSLocked() {return getCurrentScene()->fpsLock;};

        UIManager& getUIManager();
};

#endif