#ifndef SCENE_H
#define SCENE_H

#include <memory>
#include <rendering/opengl/shaders.hpp>
#include <rendering/mesh.hpp>
#include <ui/font.hpp>
#include <ui/core.hpp>
#include <queue>
#include <ui/opengl_backend.hpp>

class SceneManager;

/**
 * @brief A generic scene
 * 
 */
class Scene{
    protected:
        UIWindowIdentifier windowID;
        SceneManager* sceneManager;

        bool fpsLock = true;
        int targetFPS = 120;

    public:
        Scene(){}
        void setUILayer(const std::string& name);
        UILayer& getCurrentUILayer();
        UILayer& getUILayer(const std::string& name);
        bool isActiveLayer(const std::string& name);
        void addElement(std::shared_ptr<UIFrame> element);
        UIWindow* getWindow();
        
    protected:
        virtual void initialize() {};
        virtual void render() {};
        virtual void open(GLFWwindow* window)  {};
        virtual void close(GLFWwindow* window)  {};
        virtual void resize(GLFWwindow* /*window*/, int /*width*/, int /*height*/)  {};

        virtual void mouseMove(GLFWwindow* /*window*/, int /*x*/, int /*y*/)  {};
        virtual void mouseEvent(GLFWwindow* /*window*/, int /*button*/, int /*action*/, int /*mods*/)  {};
        virtual void scrollEvent(GLFWwindow* /*window*/, double /*xoffset*/, double /*yoffset*/) {};

        virtual void keyEvent(GLFWwindow* /*window*/, int /*key*/, int /*scancode*/, int /*action*/, int /*mods*/) {};
        
        friend class SceneManager;
};

/**
 * @brief A manager that hold all existing scenes
 * 
 */
class SceneManager{ 
    private:
        std::string currentScene = "internal_default";
        std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;

        UIOpenglBackend opengl_backend;

        GLFWwindow* window;
        UIEventLock eventLocks;

    public:
        SceneManager(GLFWwindow* window);

        template <typename T>
        T* createScene(const std::string& name){
            auto scene = std::make_unique<T>();
            T* scene_ptr = scene.get();
            addScene(name, std::move(scene));
            scene_ptr->initialize();
            return scene_ptr;
        }
        
        void addScene(const std::string& name, std::unique_ptr<Scene> scene);
        void setScene(const std::string& name);
        Scene* getScene(const std::string& name);
        Scene* getCurrentScene();
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
};

#endif