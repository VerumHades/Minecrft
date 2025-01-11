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
    addElement(uiManager->createElement<TestWidget>());
    setUILayer("second");
    auto second = uiManager->createElement<TestWidget>();
    second->setPosition(100,100);
    addElement(second);
    setUILayer("second");
}

void TestWidget::getRenderingInformation(UIRenderBatch& batch){
    batch.Rectangle(transform.x,transform.y,100,100,{255,0,0});
    batch.Text("aoiwndakijbndwkja,cfklmasdnfviqauejhfnbakjfbn,zxmcplnkcqWA I EDquf", transform.x, transform.y, 20,  {255,255,255});
}