#include <scene.hpp>

/*
    Adds an element to the current selected ui layer
*/
void Scene::addElement(std::shared_ptr<UIFrame> element){
    this->uiManager->getWindow(windowID).getCurrentLayer().addElement(element);
    this->uiManager->update();
}
/*
    Sets the current selected ui layer
*/
void Scene::setUILayer(std::string name){
    this->uiManager->getWindow(windowID).setCurrentLayer(name);
    this->uiManager->update();

    auto& layer = getCurrentUILayer();
    glfwSetInputMode(sceneManager->getGLFWWindow(), GLFW_CURSOR, layer.cursorMode);
    sceneManager->setEventLocks(layer.eventLocks);

    uiManager->setFocus(nullptr);
}
UILayer& Scene::getCurrentUILayer(){
    return this->uiManager->getWindow(windowID).getCurrentLayer();
}
UILayer& Scene::getUILayer(std::string name){
    return this->uiManager->getWindow(windowID).getLayer(name);
}

SceneManager::SceneManager(){
    std::unique_ptr<Scene> defaultScene = std::make_unique<Scene>();
    addScene("internal_default", std::move(defaultScene));
    setScene("internal_default");

    auto label = std::make_unique<UILabel>(
        "This is the default scene, if you are seeing this something has gone wrong!", 
        TValue(OPERATION_MINUS,{FRACTIONS, 50}, {MFRACTION, 50}),
        TValue(OPERATION_MINUS,{FRACTIONS, 50}, {MFRACTION, 50}),
        TValue(PIXELS, 200),
        TValue(PIXELS, 40),
        glm::vec4(0,0,0,1)
    );

    getCurrentScene()->addElement(std::move(label));

    auto label2 = std::make_unique<UILabel>(
        "Or you could be a dev, in which case you should have intended this to happen.", 
        TValue(OPERATION_MINUS,{FRACTIONS, 50}, {MFRACTION, 50}),
        TValue(OPERATION_PLUS ,{FRACTIONS, 50}, {MFRACTION, 70}),
        TValue(PIXELS, 200),
        TValue(PIXELS, 40),
        glm::vec4(0,0,0,1)
    );

    /*label2->onMouseEvent = [](GLFWwindow* window, int button, int action, int mods) {
        std::cout << "Hello World!" << std::endl;
    };*/

    getCurrentScene()->addElement(std::move(label2));
}

void SceneManager::initialize(){
    manager.initialize();
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
    scenes[name]->uiManager = &manager;
    scenes[name]->sceneManager = this;
    scenes[name]->windowID = manager.createWindow();
}

void SceneManager::setScene(std::string name){
    if(currentScene != name) {
        getCurrentScene()->close(window);
    }
    else return;
    currentScene = name;
    getCurrentScene()->open(window);
    manager.setCurrentWindow(getCurrentScene()->windowID);
    getCurrentScene()->setUILayer("default");
}

void SceneManager::resize(GLFWwindow* window, int width, int height){
    glViewport(0,0,width,height);
    getCurrentScene()->resize(window,width,height);
    manager.resize(width,height);
}
void SceneManager::mouseMove(GLFWwindow* window, int x, int y){
    manager.mouseMove(x,y);
    if(!eventLocks.mouseMove) getCurrentScene()->mouseMove(window, x,y);
}

void SceneManager::mouseEvent(GLFWwindow* window, int button, int action, int mods){
    manager.mouseEvent(window,button,action,mods);
    if(!eventLocks.mouseEvent) getCurrentScene()->mouseEvent(window,button,action,mods);
}
void SceneManager::scrollEvent(GLFWwindow* window, double xoffset, double yoffset){
    if(!eventLocks.scrollEvent) getCurrentScene()->scrollEvent(window, xoffset, yoffset);
}

void SceneManager::keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods){
    manager.keyEvent(window,key,scancode,action,mods);
    if(!eventLocks.keyEvent) getCurrentScene()->keyEvent(window,key,scancode,action,mods);
    getCurrentScene()->unlockedKeyEvent(window,key,scancode,action,mods);
}

void SceneManager::render(){
    getCurrentScene()->render();
    manager.render();
}

void SceneManager::keyTypedEvent(GLFWwindow* window, unsigned int codepoint){
    manager.keyTypedEvent(window, codepoint);
}