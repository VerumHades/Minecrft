#include <scene.hpp>

SceneManager::SceneManager(){
    std::unique_ptr<Scene> defaultScene = std::make_unique<Scene>();
    
    auto label = std::make_unique<UILabel>(
        "This is the default scene, if you are seeing this something has gone wrong!", 
        TValue(OPERATION_MINUS,{FRACTIONS, 50}, {MFRACTION, 50}),
        TValue(OPERATION_MINUS,{FRACTIONS, 50}, {MFRACTION, 50}),
        glm::vec3(0,0,0)
    );

    defaultScene->window.getCurrentLayer().elements.push_back(std::move(label));

    auto label2 = std::make_unique<UILabel>(
        "Or you could be a dev, in which case you should have intended this to happen.", 
        TValue(OPERATION_MINUS,{FRACTIONS, 50}, {MFRACTION, 50}),
        TValue(OPERATION_PLUS ,{FRACTIONS, 50}, {MFRACTION, 70}),
        glm::vec3(0,0,0)
    );

    /*label2->onMouseEvent = [](GLFWwindow* window, int button, int action, int mods) {
        std::cout << "Hello World!" << std::endl;
    };*/

    defaultScene->window.getCurrentLayer().elements.push_back(std::move(label2));

    addScene("internal_default", std::move(defaultScene));
    setScene("internal_default");
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

void SceneManager::addScene(std::string name, std::unique_ptr<Scene> scene){
    if(scenes.count(name) != 0){
        std::cout << "Scene under name: " << currentScene << " already exists." << std::endl;
        return;
    }

    scenes[name] = std::move(scene);
    scenes[name]->manager = &manager;
}

void SceneManager::setScene(std::string name){
    if(currentScene != name) {
        getCurrentScene()->close(window);
    }
    else return;
    currentScene = name;
    getCurrentScene()->open(window);
    manager.setCurrentWindow(&getCurrentScene()->window);
}

void SceneManager::resize(GLFWwindow* window, int width, int height){
    glViewport(0,0,width,height);
    getCurrentScene()->resize(window,width,height);
    manager.resize(width,height);
}
void SceneManager::mouseMove(GLFWwindow* window, int x, int y){
    getCurrentScene()->mouseMove(window, x,y);
    manager.mouseMove(x,y);
}

void SceneManager::mouseEvent(GLFWwindow* window, int button, int action, int mods){
    getCurrentScene()->mouseEvent(window,button,action,mods);
    manager.mouseEvent(window,button,action,mods);
}
void SceneManager::scrollEvent(GLFWwindow* window, double xoffset, double yoffset){
    getCurrentScene()->scrollEvent(window, xoffset, yoffset);
}

void SceneManager::keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods){
    getCurrentScene()->keyEvent(window,key,scancode,action,mods);
}

void SceneManager::render(){
    getCurrentScene()->render();
    manager.render();
}
