#include <test_scene.hpp>

//void TestScene::unlockedKeyEvent(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/){
//    if(key == GLFW_KEY_W && action == GLFW_PRESS){
//        std::cout << "Down" << std::endl;
//
//        if(stage) setUILayer("second");
//        else setUILayer("first");
//
//        stage = !stage;
//    }
//}

void TestScene::initialize(){
    setUILayer("first");
}

void TestScene::open(GLFWwindow* window){
    setUILayer("first");
}