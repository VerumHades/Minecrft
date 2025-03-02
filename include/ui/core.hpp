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
#include <sol/sol.hpp>

class UICore;
class UIFrame;
class UILoader;
class UIStyle;

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
        std::string name = "none";
        std::function<void()> onEntered;

        void clear(){elements.clear();}
        void addElement(std::shared_ptr<UIFrame> element){
            elements.push_back(element);
            //element->calculateTransforms();
        }
        void addElementWithID(std::string id, std::shared_ptr<UIFrame> element){
            idRegistry[id] = element;
        }

        std::vector<std::shared_ptr<UIFrame>>& getElements() {return elements;}
};

using UIWindowIdentifier = int;

class UIWindow{
    private: 
        std::unordered_map<std::string, std::shared_ptr<UILayer>> layers;
        std::string currentLayer = "default";

    public:
        void setCurrentLayer(std::string name) {currentLayer = name;};
        std::string getCurrentLayerName(){return currentLayer;}

        void addLayer(std::shared_ptr<UILayer> layer){
            if(!layer) return;
            layers[layer->name] = layer;
        }
        UILayer& getCurrentLayer() {
            return getLayer(currentLayer);
        }
        UILayer& getLayer(std::string name) {
            if(layers.contains(name)) return *layers.at(name);
            
            auto& layer = layers[name];
            layer = std::make_shared<UILayer>();
            layer->name = name;
            return *layer;
        }
};
#include <ui/loader.hpp>

class UICore{
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

        UILoader loader_;
        UIBackend* backend = nullptr;

        sol::state _lua;
        UICore();

        std::unordered_map<std::string, std::shared_ptr<GLTextureArray>> loaded_images{};

    public:
        void cleanup();

        void setBackend(UIBackend* backend);
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

        UILoader& loader() {return loader_;}
        sol::state& lua(){return _lua;}
        UIBackend& getBackend();

        std::shared_ptr<UIFrame> getElementUnder(int x, int y, bool onlyScrollable = false);   

        glm::ivec2 getMousePosition(){return mousePosition;}

        int getScreenWidth() {return screenWidth;}
        int getScreenHeight() {return screenHeight;}

        static UICore& get();
        static std::shared_ptr<GLTextureArray> LoadImage(const std::string& path);
        static std::shared_ptr<GLTextureArray> LoadImage(const Image& image);
};


template<typename T>
static inline auto getElementById(const std::string& id){
    return UICore::get().loader().getElementById<T>(id);
}

#include <ui/elements.hpp>

#endif