#include <scene.hpp>

/*
    Adds an element to the current selected ui layer
*/
void Scene::addElement(std::shared_ptr<UIFrame> element){
    UICore::get().getWindow(windowID)->getCurrentLayer().addElement(element);
}
/*
    Sets the current selected ui layer
*/
void Scene::setUILayer(std::string name){
    UICore::get().stopDrawingAll();
    UICore::get().getWindow(windowID)->setCurrentLayer(name);

    auto& layer = getCurrentUILayer();
    glfwSetInputMode(sceneManager->getGLFWWindow(), GLFW_CURSOR, layer.cursorMode);
    sceneManager->setEventLocks(layer.eventLocks);
    
    UICore::get().resetStates();
    UICore::get().updateAll();
}

bool Scene::isActiveLayer(std::string name){
    return getCurrentUILayer().name == name;
}

UIWindow* Scene::getWindow(){
    return UICore::get().getWindow(windowID);
}

UILayer& Scene::getCurrentUILayer(){
    return UICore::get().getWindow(windowID)->getCurrentLayer();
}
UILayer& Scene::getUILayer(std::string name){
    return UICore::get().getWindow(windowID)->getLayer(name);
}

SceneManager::SceneManager(GLFWwindow* window): window(window){
    UICore::get().setBackend(&opengl_backend);

    std::unique_ptr<Scene> defaultScene = std::make_unique<Scene>();
    addScene("internal_default", std::move(defaultScene));
    setScene("internal_default");

    auto label = std::make_shared<UILabel>();
    label->setPosition(
        TValue(OPERATION_MINUS,{WINDOW_WIDTH, 50}, {MY_PERCENT, 50}),
        TValue(OPERATION_MINUS,{WINDOW_WIDTH, 50}, {MY_PERCENT, 50})
    );
    label->setSize(
        TValue(PIXELS, 200),
        TValue(PIXELS, 40)
    );
    label->setText("This is the default scene, if you are seeing this something has gone wrong!");

    getCurrentScene()->addElement(std::move(label));
}

Scene* SceneManager::getCurrentScene(){
    if(scenes.count(currentScene) == 0){
        std::cout << "No scene: " << currentScene << " exists. It will crash." << std::endl;
        throw std::runtime_error("Scene manager has to have a valid active scene!");
    }

    return scenes.at(currentScene).get();
}

Scene* SceneManager::getScene(std::string name){
    if(scenes.count(name) == 0){
        std::cout << "No scene: " << currentScene << " exists." << std::endl;
    }

    return scenes.at(name).get();
}

void SceneManager::addScene(std::string name, std::unique_ptr<Scene> scene){
    if(scenes.count(name) != 0){
        std::cout << "Scene under name: " << currentScene << " already exists." << std::endl;
        return;
    }

    scenes[name] = std::move(scene);
    scenes[name]->sceneManager = this;
    scenes[name]->windowID = UICore::get().createWindow();
}

void SceneManager::setScene(std::string name){
    if(currentScene != name) {
        getCurrentScene()->close(window);
    }
    else return;
    currentScene = name;
    getCurrentScene()->open(window);
    UICore::get().setCurrentWindow(getCurrentScene()->windowID);
    getCurrentScene()->setUILayer("default");
    UICore::get().updateAll();
}

void SceneManager::resize(GLFWwindow* window, int width, int height){
    glViewport(0,0,width,height);
    getCurrentScene()->resize(window,width,height);
    UICore::get().resize(width,height);
}
void SceneManager::mouseMove(GLFWwindow* window, int x, int y){
    UICore::get().mouseMove(x,y);
    if(!eventLocks.mouseMove) getCurrentScene()->mouseMove(window, x,y);
}

void SceneManager::mouseEvent(GLFWwindow* window, int button, int action, int mods){
    UICore::get().mouseEvent(window,button,action,mods);
    if(!eventLocks.mouseEvent) getCurrentScene()->mouseEvent(window,button,action,mods);
}
void SceneManager::scrollEvent(GLFWwindow* window, double xoffset, double yoffset){
    UICore::get().scrollEvent(window, xoffset, yoffset);
    if(!eventLocks.scrollEvent) getCurrentScene()->scrollEvent(window, xoffset, yoffset);
}

void SceneManager::keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods){
    UICore::get().keyEvent(window,key,scancode,action,mods);
    if(!eventLocks.keyEvent) getCurrentScene()->keyEvent(window,key,scancode,action,mods);
}

void SceneManager::render(){
    getCurrentScene()->render();
    UICore::get().render();
}

void SceneManager::keyTypedEvent(GLFWwindow* window, unsigned int codepoint){
    UICore::get().keyTypedEvent(window, codepoint);
}